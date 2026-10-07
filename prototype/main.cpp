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

struct ScreenCaptureAttr {
    Display *display;
    Window root;
    XWindowAttributes window_attributes;
    XShmSegmentInfo shminfo;
    XImage *image;
};

ScreenCaptureAttr SetUpScreenCapture(unsigned int width, unsigned int height);
Mat GetFrame(ScreenCaptureAttr *at, int roi_x, int roi_y, unsigned int width, unsigned int height);

int main() {
    // Regiao de interesse
    int roi_x = 234;
    int roi_y = 140;
    unsigned int width = 1503;
    unsigned int height = 750;
    int fps = 60;

    // Obs: essa imagem tem dimensões exatas de 1503x750
    Mat background = imread("images/fundo.png", IMREAD_GRAYSCALE);
    if (background.empty()) return -1;

    ScreenCaptureAttr cap_attr = SetUpScreenCapture(width, height);
    // cv::VideoWriter writer("saida.avi", cv::VideoWriter::fourcc('M','J','P','G'),
    //                            fps, cv::Size(width, height));

    std::cout << "Gravando... Pressione Ctrl+C no terminal ou 'q' na janela para parar." << std::endl;

    auto frame_duration = std::chrono::milliseconds(1000 / fps);
    bool running = true;

    while (running) {
        auto start_time = std::chrono::steady_clock::now();
        Mat current_frame_gray = GetFrame(&cap_attr, roi_x, roi_y, width, height);

        vector<Puffle> puffles;

        Mat diff_image, mask;

        absdiff(current_frame_gray, background, diff_image);
        threshold(diff_image, mask, 30, 255, THRESH_BINARY);

        vector<vector<Point>> contours;
        findContours(mask, contours, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);

        for (size_t i = 0; i < contours.size(); i++) {
            double area = contourArea(contours[i]);
            if (area > 10000.0) {
                Moments m = moments(contours[i]);
                int puffle_x = m.m10 / m.m00;
                int puffle_y = m.m01 / m.m00;

                puffles.push_back(Puffle{puffle_x, puffle_y});

                // 1. Desenha o contorno exato do objeto detectado (em Verde)
                drawContours(current_frame_gray, contours, (int)i, Scalar(0, 255, 0), 2);

                // 2. Extrai e desenha a Bounding Box (Caixa de Colisão) (em Azul)
                Rect bounding_box = boundingRect(contours[i]);
                rectangle(current_frame_gray, bounding_box, Scalar(255, 0, 0), 2);


                // putText(debug_frame, to_string(area), Point(center_x, center_y), FONT_HERSHEY_SIMPLEX, 1.0, Scalar(0, 0, 255));
                // 3. Desenha um círculo preenchido no Centroide exato (em Vermelho)
                circle(current_frame_gray, Point(puffle_x, puffle_y), 5, Scalar(0, 0, 255), -1);

                // writer.write(current_frame_gray);
            }
        }

        // Após coletar as imagens, pegar o mais próximo da raquete:
        Puffle *closest_puffle = &puffles[0];

        for (int i = 1; i < puffles.size(); i++) {
            if (puffles[i].pos.y > closest_puffle->pos.y) {
                closest_puffle = &puffles[i];
            }
        }
        // cout << "Num Puffles: " << puffles.size() << endl;
        // cout << "Closest: " << "(" << closest_puffle->pos.x << ", " << closest_puffle->pos.y << ")" << endl;
        // imshow("bosta", current_frame_gray);

        if (waitKey(16) == 'q') {
            running = false;
        }

        // Controlar a taxa de quadros (FPS)
        auto end_time = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        if (elapsed < frame_duration) {
            std::this_thread::sleep_for(frame_duration - elapsed);
        }
    }

    // Limpeza dos recursos
    destroyAllWindows();
    // writer.release();

    XShmDetach(cap_attr.display, &cap_attr.shminfo);
    XDestroyImage(cap_attr.image);
    shmdt(cap_attr.shminfo.shmaddr);
    shmctl(cap_attr.shminfo.shmid, IPC_RMID, 0);
    XCloseDisplay(cap_attr.display);

    cout << "Gravação finalizada." << endl;
    return 0;
}

ScreenCaptureAttr SetUpScreenCapture(unsigned int width, unsigned int height)
{
    ScreenCaptureAttr at;
    // 2. Conectar ao servidor X11
    at.display = XOpenDisplay(nullptr);
    if (!at.display) {
        std::cerr << "Erro: Não foi possível abrir o display X11." << std::endl;
        exit(1);
    }

    at.root = DefaultRootWindow(at.display);
    XGetWindowAttributes(at.display, at.root, &at.window_attributes);

    // 3. Configurar a Memória Compartilhada (XShm)
    at.image = XShmCreateImage(at.display, at.window_attributes.visual,
                                    at.window_attributes.depth, ZPixmap, nullptr,
                                    &at.shminfo, width, height);

    // Alocar a memória compartilhada
    at.shminfo.shmid = shmget(IPC_PRIVATE, at.image->bytes_per_line * at.image->height, IPC_CREAT | 0777);
    at.shminfo.shmaddr = at.image->data = (char*)shmat(at.shminfo.shmid, 0, 0);
    at.shminfo.readOnly = False;

    // Anexar a memória ao X Server
    if (!XShmAttach(at.display, &at.shminfo)) {
        std::cerr << "Erro: Falha ao anexar a memória compartilhada do X11." << std::endl;
        exit(1);
    }

    return at;
}


Mat GetFrame(ScreenCaptureAttr *at, int roi_x, int roi_y, unsigned int width, unsigned int height)
{
    XShmGetImage(at->display, at->root, at->image, roi_x, roi_y, AllPlanes);

    // O X11 retorna os pixels no formato BGRA (4 canais). O OpenCV precisa associar isso a um cv::Mat.
    cv::Mat frame_bgra(height, width, CV_8UC4, at->image->data);
    cv::Mat frame_gray;

    // Converter BGRA para BGR (formato padrão do VideoWriter)
    cv::cvtColor(frame_bgra, frame_gray, cv::COLOR_RGBA2GRAY);

    return frame_gray;
}
