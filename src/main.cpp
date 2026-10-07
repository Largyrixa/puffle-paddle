#include <X11/X.h>
#include <opencv2/core/mat.hpp>
#include <opencv2/core/types.hpp>
#include <opencv2/flann/defines.h>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/opencv.hpp>
#include <iostream>
#include <opencv2/videoio.hpp>
#include <vector>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/XShm.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <chrono>
#include <thread>

#define MASK_THRESH 40 // Experimentalmente o melhor valor

using namespace cv;
using namespace std;

class Puffle {
public:
    Point pos;
    Scalar color;

    Puffle(): pos{0, 0}, color{0, 0, 0} {}
    Puffle(int _x, int _y): pos{_x, _y}, color{0,0,0} {}
    Puffle(Point _pos, Scalar _color): pos{_pos}, color{_color} {}
};

class ScreenCapture {
public:
    Display *display;
    Window root;
    XWindowAttributes window_attributes;
    XShmSegmentInfo shminfo;
    XImage *image;
    int roi_x, roi_y;
    unsigned int width, height;

    ScreenCapture(int _roi_x, int _roi_y, unsigned int _w, unsigned int _h);
    cv::Mat get_frame();
    ~ScreenCapture();
};

void MouseCursorGoto(Display *, Window, int, int);
int  GetKeyPressed(Display *);

int main() {
    // Regiao de interesse
    int roi_x = 234;
    int roi_y = 140;
    unsigned int width = 1503;
    unsigned int height = 750;
    int fps = 60;

    // Obs: essa imagem tem dimensões exatas de 1503x750
    Mat background = imread("images/background.png", IMREAD_GRAYSCALE);
    if (background.empty()) return -1;

    // Setup do x11 para capturar video
    ScreenCapture cap{roi_x, roi_y, width, height};

    std::cout << "Gravando... Pressione Ctrl+C no terminal ou 'q' na janela para parar." << std::endl;
    auto frame_duration = std::chrono::milliseconds{1000 / fps};
    int frame_c = 0;
    bool running = true;
    bool move_cursor_enabled = false;

    Point closest_obj{0, 0};
    while (running) {
        auto start_time = std::chrono::steady_clock::now();
        Mat current_frame_gray = cap.get_frame();
        Mat current_frame_small;

        closest_obj.y = 0;
        Mat diff_image, mask;

        absdiff(current_frame_gray, background, diff_image);
        threshold(diff_image, mask, MASK_THRESH, 255, THRESH_BINARY);

        vector<vector<Point>> contours;
        findContours(mask, contours, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);

        for (size_t i = 0; i < contours.size(); i++) {
            double area = contourArea(contours[i]);
            if (area > 10000.0) {
                Moments m = moments(contours[i]);
                int puffle_x = m.m10 / m.m00;
                int puffle_y = m.m01 / m.m00;

                if (puffle_y > closest_obj.y) {
                    closest_obj.x = puffle_x;
                    closest_obj.y = puffle_y;
                }

                // Extrai e desenha a Bounding Box
                Rect bounding_box = boundingRect(contours[i]);
                rectangle(current_frame_gray, bounding_box, Scalar(255, 0, 0), 4);
            }
        }

        // Manda o mouse para o objeto mais próximo
        // Coordenada y é fixa, só se move a coordenada x
        if (move_cursor_enabled) {
            MouseCursorGoto(cap.display, cap.root, roi_x+closest_obj.x, roi_y+height);

        }

        resize(current_frame_gray, current_frame_small, Size{ (int)width/4, (int)height/4 });
        imshow("Captura de Tela", current_frame_small);

        int pressed_key = cv::waitKey(1);

        switch (pressed_key) {
        case 'q':
        case 'Q':
            running = false;
            break;

        case 'y':
        case 'Y':
            move_cursor_enabled = true;
            break;

        case 'n':
        case 'N':
            move_cursor_enabled = false;
            break;

        default: break;
        }

        // Controlar a taxa de quadros (FPS)
        auto end_time = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        if (++frame_c >= 60) {
            long current_fps = 1000 / elapsed.count();
            if (current_fps > 60) {
                current_fps = 60;
            }
            cout << "FPS: " << current_fps << "\r" << std::flush;
            frame_c = 0;
        }
        if (elapsed < frame_duration) {
            std::this_thread::sleep_for(frame_duration - elapsed);
        }
    }
    cout << endl;

    // Limpeza dos recursos
    destroyAllWindows();
    // ~cap();
    return 0;
}

ScreenCapture::ScreenCapture(int _roi_x, int _roi_y, unsigned int _w, unsigned int _h):
roi_x{_roi_x}, roi_y{_roi_y}, width{_w}, height{_h}
{
    display = XOpenDisplay(nullptr);
    if (!display) {
        std::cerr << "Erro: Não foi possível abrir o display X11." << std::endl;
        exit(1);
    }

    root = DefaultRootWindow(display);
    XGetWindowAttributes(display, root, &window_attributes);

    // Configurar a Memória Compartilhada (XShm)
    image = XShmCreateImage(display, window_attributes.visual,
                                    window_attributes.depth, ZPixmap, nullptr,
                                    &shminfo, width, height);

    // Alocar a memória compartilhada
    shminfo.shmid = shmget(IPC_PRIVATE, image->bytes_per_line * image->height, IPC_CREAT | 0777);
    shminfo.shmaddr = image->data = (char*)shmat(shminfo.shmid, 0, 0);
    shminfo.readOnly = False;

    // Anexar a memória ao X Server
    if (!XShmAttach(display, &shminfo)) {
        std::cerr << "Erro: Falha ao anexar a memória compartilhada do X11." << std::endl;
        exit(1);
    }
}

cv::Mat ScreenCapture::get_frame()
{
    XShmGetImage(display, root, image, roi_x, roi_y, AllPlanes);

    // O X11 retorna os pixels no formato BGRA (4 canais). O OpenCV precisa associar isso a um cv::Mat.
    cv::Mat frame_bgra(height, width, CV_8UC4, image->data);
    cv::Mat frame_gray;

    // Converter BGRA para BGR (formato padrão do VideoWriter)
    cv::cvtColor(frame_bgra, frame_gray, cv::COLOR_RGBA2GRAY);

    return frame_gray;
}

ScreenCapture::~ScreenCapture()
{
    XShmDetach(display, &shminfo);
    XDestroyImage(image);
    shmdt(shminfo.shmaddr);
    shmctl(shminfo.shmid, IPC_RMID, 0);
    XCloseDisplay(display);
    std::cout << "Gravação finalizada!" << endl;
}

void MouseCursorGoto(Display *dsp, Window w, int x, int y)
{
    XWarpPointer(
        dsp,              // Display handle
        None,             // Source window (None = use root window)
        w,                // Destination window (root window)
        0, 0, 0, 0,       // Source coordinates and size (ignored for None)
        x, y              // Destination coordinates
    );
}
