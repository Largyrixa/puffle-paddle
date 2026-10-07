# Puffle Paddle

[English](README.md) | **Português (BR)**

Autoplayer para o **Puffle Paddle**, um minigame de um evento do Club Penguin.

O programa captura uma região fixa da tela, roda visão computacional nela (subtração de fundo → threshold → detecção de contornos), encontra o puffle mais baixo na tela e move o mouse para essa posição em x, para que a raquete fique embaixo dele.

![Puffle Paddle rodando com caixas de detecção](images/exemplo-detected.png)

> [!NOTE]
> Projeto de estudo de visão computacional e automação de tela. Use por sua conta e risco.

---

## Como funciona

Todo o pipeline roda com meta de 60 FPS:

1. **Captura de tela** de uma região de interesse (ROI) fixa — `XShm` (memória compartilhada) no C++, `mss` no Python.
2. **Subtração de fundo** — `cv::absdiff` entre o quadro atual e `images/background.png`.
3. **Threshold** — converte a imagem de diferença em uma máscara binária (`MASK_THRESH`).
4. **Detecção de contornos** — contornos externos menores que `OBJECT_MIN_AREA` são ignorados (ruído, sprites pequenos).
5. **Centroide do objeto mais baixo** — vence o contorno com maior `y`; esse é o puffle mais perto de cair.
6. **Controle do mouse** — o x do cursor segue o centroide do puffle; o y fica fixo no fundo da ROI. Desligado por padrão, ligado pelo teclado.

Uma janela de debug mostra a ROI com uma bounding box desenhada em cada objeto detectado.

## Implementações

| | C++ | Python |
|---|---|---|
| Ponto de entrada | [`src/main.cpp`](src/main.cpp) | [`src/main.py`](src/main.py) |
| Captura de tela | X11 / XShm | `mss` |
| Controle do mouse | `XWarpPointer` | `pyautogui` |
| Dependências | OpenCV, X11 | `opencv-python`, `mss`, `numpy`, `pyautogui` |
| Plataforma | Linux (X11) | Multiplataforma |

As duas versões compartilham as mesmas constantes, a mesma ROI e o mesmo comportamento — a versão Python é um port direto da C++.

## Requisitos

**C++**

- CMake ≥ 3.10 e um compilador C++17
- OpenCV
- Bibliotecas de desenvolvimento do X11

No Debian/Ubuntu:

```bash
sudo apt install build-essential cmake libopencv-dev libx11-dev libxext-dev
```

**Python**

- Python 3.x
- Dependências listadas no [`requirements.txt`](requirements.txt)

## Como rodar

### C++

```bash
cmake -B build
./run.sh
```

Ou compile manualmente:

```bash
cmake -B build
cmake --build build
./build/main
```

### Python

```bash
pip install -r requirements.txt
python src/main.py
```

> Execute a partir da raiz do repositório — o programa carrega `images/background.png` com caminho relativo.

## Controles

O controle do mouse começa **desligado**, para você poder posicionar a janela do jogo primeiro:

| Tecla | Ação |
|---|---|
| `Y` | Liga o controle do mouse |
| `N` | Desliga o controle do mouse |
| `Q` | Sai do programa |
| `Ctrl+C` | Sai do programa (terminal) |

## Configuração

Esses valores ficam no topo do [`src/main.cpp`](src/main.cpp) e do [`src/main.py`](src/main.py):

| Constante | Padrão | Descrição |
|---|---|---|
| `roi_x`, `roi_y` | `234`, `140` | Canto superior esquerdo da região de interesse na tela |
| `width`, `height` | `1503`, `750` | Tamanho da região de interesse |
| `MASK_THRESH` | `40` | Threshold aplicado à imagem de diferença do fundo |
| `OBJECT_MIN_AREA` | `10000` | Área mínima do contorno para contar como puffle |
| `fps` | `60` | Taxa de quadros alvo |

> [!IMPORTANT]
> `images/background.png` precisa ser capturado com as dimensões **exatas** da ROI (`1503×750`) e sem puffles visíveis, senão a detecção quebra.

## Estrutura do projeto

```
├── CMakeLists.txt      # Build da versão C++
├── requirements.txt    # Dependências da versão Python
├── run.sh              # Compila e executa a versão C++
├── images/
│   ├── background.png  # Fundo de referência (sem puffles)
│   └── exemplo*.png    # Screenshots de exemplo
├── src/
│   ├── main.cpp        # Versão C++ (X11 + OpenCV)
│   └── main.py         # Versão Python (mss + pyautogui + OpenCV)
└── test/
    └── test.py         # Scripts antigos de experimentação
```
