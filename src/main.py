import time

import cv2
import mss
import numpy as np
import pyautogui

# Constantes de configuração
MASK_THRESH = 40
OBJECT_MIN_AREA = 10000.0

# Região de interesse (ROI)
# MSS usa um dicionário para definir as coordenadas
roi_x = 234
roi_y = 140
width = 1503
height = 750
monitor = {"top": roi_y, "left": roi_x, "width": width, "height": height}

# Configurações Críticas do PyAutoGUI
# PyAutoGUI tem um delay padrão de 0.1s após cada comando. Precisamos zerar isso.
pyautogui.PAUSE = 0
# Move o mouse instantaneamente sem animação
pyautogui.MINIMUM_DURATION = 0

def main():
    # Carregar o background (Obs: deve ter as dimensões exatas de 1503x750)
    background = cv2.imread("images/background.png", cv2.IMREAD_GRAYSCALE)
    if background is None:
        print("Erro: Não foi possível carregar images/background.png")
        return

    # Instanciar a captura de tela
    sct = mss.mss()

    print("Gravando... Pressione 'q' na janela do OpenCV para parar.")

    fps = 60
    frame_duration = 1.0 / fps
    frame_c = 0
    move_cursor_enabled = False

    running = True
    fps_start_time = time.perf_counter()

    while running:
        start_time = time.perf_counter()

        # 1. Captura de tela via mss
        sct_img = sct.grab(monitor)

        # O mss retorna BGRA. Convertendo para um array do NumPy e depois para Cinza
        frame_bgra = np.array(sct_img)
        current_frame_gray = cv2.cvtColor(frame_bgra, cv2.COLOR_BGRA2GRAY)

        # Criar uma cópia colorida para visualizar a Bounding Box colorida no debug
        debug_frame = cv2.cvtColor(current_frame_gray, cv2.COLOR_GRAY2BGR)

        # 2. Visão Computacional (Subtração e Threshold)
        diff_image = cv2.absdiff(current_frame_gray, background)
        _, mask = cv2.threshold(diff_image, MASK_THRESH, 255, cv2.THRESH_BINARY)

        contours, _ = cv2.findContours(mask, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)

        closest_obj_x = 0
        closest_obj_y = 0

        # 3. Processamento dos Contornos
        for contour in contours:
            area = cv2.contourArea(contour)
            if area > OBJECT_MIN_AREA:
                m = cv2.moments(contour)
                if m["m00"] != 0:
                    puffle_x = int(m["m10"] / m["m00"])
                    puffle_y = int(m["m01"] / m["m00"])

                    # Atualiza o objeto mais baixo na tela
                    if puffle_y > closest_obj_y:
                        closest_obj_x = puffle_x
                        closest_obj_y = puffle_y

                    # Extrai e desenha a Bounding Box
                    x, y, w, h = cv2.boundingRect(contour)
                    cv2.rectangle(debug_frame, (x, y), (x + w, y + h), (255, 0, 0), 4)

        # 4. Controle do Mouse
        # Coordenada y é fixa (roi_y + height), só se move a coordenada x
        if move_cursor_enabled and closest_obj_y > 0:
            target_x = roi_x + closest_obj_x
            target_y = roi_y + height
            pyautogui.moveTo(target_x, target_y)

        # 5. Debug Visual
        debug_small = cv2.resize(debug_frame, (width // 4, height // 4))
        cv2.imshow("Captura de Tela", debug_small)

        # 6. Captura de Input do Teclado
        pressed_key = cv2.waitKey(1) & 0xFF
        if pressed_key == ord('q'):
            running = False
        elif pressed_key == ord('y'):
            move_cursor_enabled = True
        elif pressed_key == ord('n'):
            move_cursor_enabled = False

        # 7. Controle preciso de FPS (usando perf_counter para maior precisão que time())
        end_time = time.perf_counter()
        elapsed = end_time - start_time

        if elapsed < frame_duration:
            time.sleep(frame_duration - elapsed)

        # Cálculo do FPS médio acumulado a cada 60 frames (mais estável que o original em C++)
        frame_c += 1
        if frame_c >= 60:
            current_time = time.perf_counter()
            real_fps = 60 / (current_time - fps_start_time)
            print(f"FPS: {min(60.0, real_fps):.0f}", end="\r", flush=True)
            frame_c = 0
            fps_start_time = current_time

    # Limpeza
    cv2.destroyAllWindows()
    print("\nGravação finalizada!")

if __name__ == "__main__":
    main()
