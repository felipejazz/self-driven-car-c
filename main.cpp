#include <SFML/Graphics.hpp>
#include <vector>
#include <string>
#include <memory>    // Para std::make_unique, std::unique_ptr
#include <iostream>
#include <algorithm> // Para std::min_element, std::find_if
#include <limits>    // Para numeric_limits
#include <cstdio>    // Para std::remove
#include <optional>  // Para std::optional (retorno de pollEvent)

// --- Headers do Projeto (Assumindo compatibilidade com SFML 3) ---
#include "Road.hpp"
#include "Car.hpp"          // Deve definir ControlType, Car::update, Car::draw, Car::brain, etc.
#include "Visualizer.hpp"   // Deve definir Visualizer::drawNetwork compatível com SFML 3
#include "Utils.hpp"        // Deve definir getRandomColor()
#include "Network.hpp"      // Deve definir NeuralNetwork e seus métodos

// --- Constantes e Definições ---
const std::string BRAIN_FILENAME = "bestBrain.dat"; // Arquivo de dados binários para o cérebro

// --- Função para Gerar Carros IA ---
std::vector<std::unique_ptr<Car>> generateCars(int N, const Road& road) {
    std::vector<std::unique_ptr<Car>> cars;
    cars.reserve(N);
    for (int i = 0; i < N; ++i) {
        // Assumindo que o construtor de Car agora aceita ControlType diretamente
        cars.push_back(std::make_unique<Car>(
            road.getLaneCenter(1), // Começa na faixa do meio
            100.0f,                // Posição Y inicial
            30.0f,                 // Largura
            50.0f,                 // Altura
            ControlType::AI,       // Tipo (Usando o enum class)
            3.0f,                  // Velocidade máxima padrão
            getRandomColor()       // Cor aleatória
        ));
    }
    return cars;
}

int main() {
    // --- Configuração da Janela ---
    const unsigned int carCanvasWidth = 200;
    const unsigned int networkCanvasWidth = 400;
    const unsigned int windowWidth = carCanvasWidth + networkCanvasWidth;
    const unsigned int windowHeight = 700;

    sf::RenderWindow window(sf::VideoMode({windowWidth, windowHeight}), "Self-Driving Car - C++/SFML");
    window.setFramerateLimit(60);

    // --- Configuração das Views (SFML 3) ---
    sf::View carView(sf::FloatRect( {0.f, 0.f}, {(float)carCanvasWidth, (float)windowHeight} ));
    carView.setViewport(sf::FloatRect({0.f, 0.f}, {(float)carCanvasWidth / windowWidth, 1.f}));

    sf::View networkView(sf::FloatRect({0.f, 0.f}, {(float)networkCanvasWidth, (float)windowHeight}));
    networkView.setViewport(sf::FloatRect({(float)carCanvasWidth / windowWidth, 0.f}, {(float)networkCanvasWidth / windowWidth, 1.f}));

    // --- Configuração da Fonte (SFML 3) ---
    sf::Font font;
    if (!font.openFromFile("assets/LiberationSans-Regular.ttf")) { // Corrigido: loadFromFile
        std::cerr << "Erro ao carregar fonte 'assets/LiberationSans-Regular.ttf'. Labels da rede podem não aparecer." << std::endl;
        // Considerar sair ou usar fallback
    }

    // --- Configuração da Simulação ---
    Road road(carCanvasWidth / 2.0f, carCanvasWidth * 0.9f, 3);

    const int N = 100;
    auto cars = generateCars(N, road);

    Car* bestCar = nullptr;
    if (!cars.empty()) {
        bestCar = cars[0].get();
    } else {
        std::cerr << "Aviso: Nenhum carro IA foi gerado." << std::endl;
    }

    // --- Carregar Melhor Cérebro ---
    bool brainLoaded = false;
    // ***********************************************************************
    // * FIX 1: Mudar tipo do vetor para std::vector<int>
    // ***********************************************************************
    const std::vector<int> networkStructure = {5, 6, 4}; // Exemplo: {Entradas (sensores), Oculta, Saídas (F/L/R/B)}
    // ***********************************************************************

    // ***********************************************************************
    // * CORREÇÃO 1 (continuação): Usar o tipo correto no construtor
    // ***********************************************************************
    NeuralNetwork loadedBrain(networkStructure); // Agora corresponde ao construtor NeuralNetwork(const std::vector<int>&)
    // ***********************************************************************

    if (loadedBrain.loadFromFile(BRAIN_FILENAME)) {
        std::cout << "Carregado melhor cérebro de " << BRAIN_FILENAME << std::endl;
        brainLoaded = true;
        // Opcional: Verificar se a estrutura carregada corresponde (pode ser complexo dependendo da implementação de loadFromFile)
    } else {
        std::cout << "Nenhum cérebro salvo encontrado ou erro ao carregar (" << BRAIN_FILENAME << "). Começando do zero." << std::endl;
    }

    // Aplica cérebro carregado/mutado aos carros
    for (size_t i = 0; i < cars.size(); ++i) {
        // Acessa o cérebro opcional do carro usando getBrain() ou acesso direto a std::optional
        NeuralNetwork* carBrainPtr = cars[i]->getBrain(); // Usa o getter definido em Car.hpp (se existir)
        // Alternativa: if (cars[i]->brain.has_value()) { NeuralNetwork& carBrain = *(cars[i]->brain); ... }

        if (cars[i] && cars[i]->useBrain && carBrainPtr) {
            if (brainLoaded) {
                *carBrainPtr = loadedBrain; // Copia o cérebro carregado
                if (i != 0) {
                    NeuralNetwork::mutate(*carBrainPtr, 0.15f); // Muta os outros
                }
            }
            // Se não carregou, eles mantêm seus cérebros aleatórios padrão
        } else if (cars[i] && cars[i]->useBrain && !carBrainPtr) {
            std::cerr << "Aviso: Carro IA #" << i << " não tem objeto de cérebro inicializado!" << std::endl;
        }
    }

    // Garante que bestCar aponte para um carro com o cérebro correto (se carregado)
    NeuralNetwork* bestCarBrainPtr = (bestCar) ? bestCar->getBrain() : nullptr;
    if (bestCar && bestCarBrainPtr && brainLoaded) {
        *bestCarBrainPtr = loadedBrain;
    }


    // --- Configuração do Tráfego ---
    std::vector<std::unique_ptr<Car>> traffic;
    // Adiciona carros DUMMY (sem IA, geralmente apenas andam para frente)
    traffic.push_back(std::make_unique<Car>(road.getLaneCenter(1), -100.0f, 30.f, 50.f, ControlType::DUMMY, 2.0f, getRandomColor()));
    traffic.push_back(std::make_unique<Car>(road.getLaneCenter(0), -300.0f, 30.f, 50.f, ControlType::DUMMY, 2.0f, getRandomColor()));
    traffic.push_back(std::make_unique<Car>(road.getLaneCenter(2), -300.0f, 30.f, 50.f, ControlType::DUMMY, 2.0f, getRandomColor()));
    traffic.push_back(std::make_unique<Car>(road.getLaneCenter(0), -500.0f, 30.f, 50.f, ControlType::DUMMY, 2.0f, getRandomColor()));
    traffic.push_back(std::make_unique<Car>(road.getLaneCenter(1), -500.0f, 30.f, 50.f, ControlType::DUMMY, 2.0f, getRandomColor()));
    traffic.push_back(std::make_unique<Car>(road.getLaneCenter(1), -700.0f, 30.f, 50.f, ControlType::DUMMY, 2.0f, getRandomColor()));
    traffic.push_back(std::make_unique<Car>(road.getLaneCenter(2), -700.0f, 30.f, 50.f, ControlType::DUMMY, 2.0f, getRandomColor()));

    // Vetor de ponteiros brutos para passar para a função update dos carros IA
    // Não precisamos de const Car* aqui, pois Car::update espera std::vector<Car*>&
    std::vector<Car*> trafficRawPtrs;
    trafficRawPtrs.reserve(traffic.size());
    for (const auto& carPtr : traffic) {
        trafficRawPtrs.push_back(carPtr.get());
    }

    // ***********************************************************************
    // * FIX 2: Criar um vetor vazio NOMEADO para passar para update dos DUMMY
    // ***********************************************************************
    std::vector<Car*> emptyTrafficList; // Vetor vazio, mas é um lvalue
    // ***********************************************************************

    sf::Clock clock;

    // --- Loop Principal ---
    while (window.isOpen()) {
        sf::Time deltaTime = clock.restart();
        // float dtSeconds = deltaTime.asSeconds(); // Pode ser útil

        // --- Manipulação de Eventos (SFML 3) ---
        while (const auto eventOpt = window.pollEvent())
        {
            const sf::Event& event = *eventOpt;

            if (event.is<sf::Event::Closed>()) {
                window.close();
            }

            if (const auto* keyPressed = event.getIf<sf::Event::KeyPressed>())
            {
                bool isCtrlOrCmd = keyPressed->control || keyPressed->system; // system é para Cmd no macOS

                if (keyPressed->code == sf::Keyboard::Key::S && isCtrlOrCmd) {
                     NeuralNetwork* brainToSave = (bestCar) ? bestCar->getBrain() : nullptr;
                    if (brainToSave) {
                        if (brainToSave->saveToFile(BRAIN_FILENAME)) {
                             std::cout << "Cérebro salvo em " << BRAIN_FILENAME << std::endl;
                        } else {
                             std::cerr << "Erro ao salvar cérebro em " << BRAIN_FILENAME << std::endl;
                        }
                     } else {
                         std::cerr << "Nenhum melhor cérebro para salvar." << std::endl;
                     }
                 } else if (keyPressed->code == sf::Keyboard::Key::D && isCtrlOrCmd) {
                    if (std::remove(BRAIN_FILENAME.c_str()) == 0) {
                        std::cout << "Arquivo de cérebro descartado: " << BRAIN_FILENAME << std::endl;
                        // Opcional: Resetar cérebros atuais para aleatório se desejar
                    } else {
                        std::cout << "Nenhum arquivo de cérebro '" << BRAIN_FILENAME << "' para descartar ou erro ao deletar." << std::endl;
                    }
                 } else if (keyPressed->code == sf::Keyboard::Key::Escape) {
                    window.close();
                 }
            }
             // Adicionar tratamento para sf::Event::KeyReleased se necessário para controles manuais
        }

        // --- Lógica de Atualização ---

        // Atualiza carros do tráfego (DUMMY)
        for (auto& carPtr : traffic) {
            // ***********************************************************************
            // * CORREÇÃO 2 (continuação): Passar o vetor nomeado (lvalue)
            // ***********************************************************************
            carPtr->update(road.borders, emptyTrafficList); // Passa o vetor vazio nomeado
            // ***********************************************************************
        }

        // Atualiza carros IA
        for (auto& carPtr : cars) {
            // Carros IA verificam contra bordas e lista de tráfego REAL
            carPtr->update(road.borders, trafficRawPtrs);
        }

        // Encontra o melhor carro (menor Y, não danificado)
        float minY = std::numeric_limits<float>::max();
        Car* currentBestCar = nullptr;
        for(const auto& carPtr : cars) {
            if (carPtr && !carPtr->damaged && carPtr->position.y < minY) {
                minY = carPtr->position.y;
                currentBestCar = carPtr.get();
            }
        }

        // Se todos danificados, pega o "melhor" dos danificados (opcional, pode ser útil para visualização)
        if (!currentBestCar && !cars.empty()) {
            minY = std::numeric_limits<float>::max();
             for(const auto& carPtr : cars) {
                 if (carPtr && carPtr->position.y < minY) {
                    minY = carPtr->position.y;
                    currentBestCar = carPtr.get();
                 }
             }
        }
        bestCar = currentBestCar;
        if (bestCar) { // Verifica se temos um melhor carro válido
            float minDistanceAhead = std::numeric_limits<float>::max();
            Car* nearestTrafficCar = nullptr;

            for (const auto& trafficCarPtr : traffic) { // Itera sobre unique_ptr no vetor traffic
                 if (trafficCarPtr) { // Verifica se o ponteiro não é nulo
                    Car* trafficCar = trafficCarPtr.get(); // Obtém o ponteiro bruto

                    // Verifica se o carro de tráfego está À FRENTE do bestCar
                    if (trafficCar->position.y < bestCar->position.y) {
                        // Calcula a distância Euclidiana entre os carros
                        float distance = std::hypot(
                            bestCar->position.x - trafficCar->position.x,
                            bestCar->position.y - trafficCar->position.y
                        );

                        // Atualiza se esta for a menor distância encontrada até agora
                        if (distance < minDistanceAhead) {
                            minDistanceAhead = distance;
                            nearestTrafficCar = trafficCar; // Guarda referência ao carro mais próximo (opcional)
                        }
                    }
                 }
            }

            // Loga a distância mínima encontrada (se algum carro foi encontrado à frente)
            if (nearestTrafficCar) { // Se encontramos um carro de tráfego à frente
                std::cout << "BestCar (Y:" << bestCar->position.y
                          << ") - Dist to nearest traffic ahead (Y:" << nearestTrafficCar->position.y
                          << "): " << minDistanceAhead << std::endl;
            } else {
                // std::cout << "BestCar (Y:" << bestCar->position.y
                //           << ") - No traffic detected ahead." << std::endl; // Log opcional
            }
        }

        // --- Desenho ---
        window.clear(sf::Color(100, 100, 100)); // Fundo cinza

        // --- Desenha Simulação (View Esquerda) ---
        window.setView(carView);
        if (bestCar) {
             // ***********************************************************************
             // * AJUSTE SFML 3: Usar {} para construir sf::Vector2f em setCenter
             // ***********************************************************************
             carView.setCenter({carCanvasWidth / 2.0f, bestCar->position.y - windowHeight * 0.3f});
             // ***********************************************************************
             window.setView(carView); // Aplica a view atualizada
        } else {
             // ***********************************************************************
             // * AJUSTE SFML 3: Usar {} para construir sf::Vector2f em setCenter
             // ***********************************************************************
             carView.setCenter({carCanvasWidth / 2.0f, windowHeight / 2.0f});
             // ***********************************************************************
             window.setView(carView);
        }

        road.draw(window);

        // Desenha tráfego
        for (const auto& carPtr : traffic) {
            if(carPtr) carPtr->draw(window); // draw(target, drawSensor=false) por padrão
        }

        // Desenha carros IA (melhor carro por último e com sensores)
        for (const auto& carPtr : cars) {
            // Desenha não-melhores carros (ou todos se bestCar for nulo) sem sensor
            if (carPtr && carPtr.get() != bestCar) {
                 carPtr->draw(window, false);
            }
        }
        if (bestCar) {
             bestCar->draw(window, true); // Desenha melhor carro COM sensor visível
        }

        // --- Desenha Rede Neural (View Direita) ---
        window.setView(networkView);
        // Opcional: Fundo para a view da rede
        // sf::RectangleShape networkBg(sf::Vector2f((float)networkCanvasWidth, (float)windowHeight));
        // networkBg.setFillColor(sf::Color::Black);
        // window.draw(networkBg);

        NeuralNetwork* brainToDraw = (bestCar) ? bestCar->getBrain() : nullptr;
        if (brainToDraw) {
            // Passa a rede por referência constante (ou cópia, se a função drawNetwork aceitar)
            Visualizer::drawNetwork(window, *brainToDraw, font,
                                    0.f, 0.f, (float)networkCanvasWidth, (float)windowHeight);
        }

        // --- Exibe o Frame ---
        window.setView(window.getDefaultView()); // Reseta para view padrão
        window.display();
    }

    // unique_ptr fará a limpeza automaticamente ao sair do escopo

    return 0;
}