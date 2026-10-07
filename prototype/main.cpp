#include <opencv2/core/types.hpp>
#include <opencv2/flann/defines.h>
#include <opencv2/imgproc.hpp>
#include <opencv2/opencv.hpp>
#include <iostream>
#include <vector>

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

int main() {
    // Obs: essa imagem tem dimensões exatas de 1503x750
    Mat background = imread("images/fundo.png", IMREAD_GRAYSCALE);
    if (background.empty()) return -1;

    vector<Puffle> puffles;
    while (true) {
        puffles.clear();
        // Carrega o frame em cinza para a matemática (mais rápido)
        Mat current_frame_gray = imread("images/exemplo.png", IMREAD_GRAYSCALE);

        // Carrega o frame em cores apenas para o desenho do debug visual
        // Mat debug_frame = imread("images/exemplo.png", IMREAD_COLOR);

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
                // drawContours(debug_frame, contours, (int)i, Scalar(0, 255, 0), 2);

                // 2. Extrai e desenha a Bounding Box (Caixa de Colisão) (em Azul)
                // Rect bounding_box = boundingRect(contours[i]);
                // rectangle(debug_frame, bounding_box, Scalar(255, 0, 0), 2);


                // putText(debug_frame, to_string(area), Point(center_x, center_y), FONT_HERSHEY_SIMPLEX, 1.0, Scalar(0, 0, 255));
                // 3. Desenha um círculo preenchido no Centroide exato (em Vermelho)
                // circle(debug_frame, Point(center_x, center_y), 5, Scalar(0, 0, 255), -1);
            }
        }

        // Após coletar as imagens, pegar o mais próximo da raquete:
        Puffle *closest_puffle = &puffles[0];

        for (int i = 1; i < puffles.size(); i++) {
            if (puffles[i].pos.y > closest_puffle->pos.y) {
                closest_puffle = &puffles[i];
            }
        }
        cout << "Num Puffles: " << puffles.size() << endl;
        cout << "Closest: " << "(" << closest_puffle->pos.x << ", " << closest_puffle->pos.y << ")" << endl;

        // imshow("Debug 1: Mascara de Movimento (Visao do Bot)", mask);

        if (waitKey(16) == 'q') {
            break;
        }
    }

    destroyAllWindows();
    return 0;
}
