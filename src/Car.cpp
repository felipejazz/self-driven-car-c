#include "Car.hpp"
#include "Utils.hpp" // Assume que Utils.hpp define M_PI ou inclui <cmath> corretamente
#include <cmath>      // Para std::sin, std::cos, std::atan2, std::hypot, std::abs, M_PI
#include <iostream>   // Para error messages (std::cerr)
#include <SFML/Graphics/RectangleShape.hpp> // <<<--- Include this for the fallback shape

// In src/Car.cpp, dentro do construtor Car::Car(...)
Car::Car(float x, float y, float w, float h, ControlType type, float maxSpd, sf::Color col)
    : position(x, y),
      width(w),
      height(h),
      maxSpeed(maxSpd),
      controlType(type),
      controls(type), // Inicializa controls com o tipo
      color(col),
      textureLoaded(false),
      sprite(texture), // Usa o membro texture
      pendulumCounter(0),       // Inicializa o contador
      lastTurnDirection(0)      // Inicializa a última direção
{
    useBrain = (controlType == ControlType::AI);

    // ... resto do construtor (loadTexture, sensor, brain setup) ...
     loadTexture("assets/car.png");
    if (!textureLoaded) { // Print warning if loading failed inside loadTexture
        std::cerr << "Warning: Failed to load car texture 'assets/car.png'. Using fallback rectangle." << std::endl;
    }

    // Sensor and Brain setup
    if (controlType != ControlType::DUMMY) {
        sensor = std::make_unique<Sensor>(*this);
        if (useBrain && sensor) { // Verifica se sensor foi criado
            // Verifica se a rede neural deve ser criada (ControlTYpe::AI)
             // A estrutura da rede é definida aqui ou carregada depois
             const std::vector<int> networkStructure = {static_cast<int>(sensor->rayCount), 6, 4};
             brain.emplace(NeuralNetwork(networkStructure)); // Cria a rede neural dentro do optional
        }
    }
}

// Modified loadTexture - now sets the flag and returns void
void Car::loadTexture(const std::string& filename) {
    textureLoaded = false; // Assume failure initially

    if (!texture.loadFromFile(filename)) { // Carrega dados no membro 'texture'
        // Error message printed here or in the constructor
        return; // Exit if loading failed
    }
    texture.setSmooth(true);
    sprite.setTexture(texture, true); // Associa sprite à textura CARREGADA

    sf::FloatRect textureRect = sprite.getLocalBounds();

    if (textureRect.size.x > 0 && textureRect.size.y > 0) {
        float scaleX = width / textureRect.size.x;
        float scaleY = height / textureRect.size.y;
        sprite.setScale({scaleX, scaleY}); // Use initializer list for Vector2f
    } else {
        sprite.setScale({1.0f, 1.0f});
        std::cerr << "Warning: Texture loaded with zero dimensions: " << filename << ". Setting scale to 1." << std::endl;
        return; // Exit if texture dimensions are invalid
    }

    sprite.setOrigin({width / 2.0f, height / 2.0f});
    sprite.setColor(color); // Set initial sprite color

    textureLoaded = true; // <<<--- Set flag to true only on success
}

// Update function remains the same
// In src/Car.cpp
void Car::update(const std::vector<std::pair<sf::Vector2f, sf::Vector2f>>& roadBorders,
    std::vector<Car*>& traffic)
{
if (damaged) { // Se já estiver danificado, não faz mais nada
return;
}

// 1. Atualizar Sensor (SEMPRE primeiro se a IA depender dele)
if (sensor) {
sensor->update(roadBorders, traffic);
}

// 2. Calcular distância para o tráfego à frente (necessário para o threshold)
//    Nota: Passamos a lista de ponteiros brutos `traffic` aqui.
float minDistanceAhead = calculateMinDistanceAhead(traffic); // Usa a função helper

// 3. Determinar Controles (Teclado ou IA)
bool intendedTurnLeft = false;
bool intendedTurnRight = false;
bool setControlsFromNN = false; // Flag para saber se a NN definiu os controles

if (controlType == ControlType::KEYS) {
controls.update(); // Atualiza baseado no teclado (já define controls.left/right diretamente)
intendedTurnLeft = controls.left;
intendedTurnRight = controls.right;
} else if (useBrain && brain && sensor) { // Se for IA com cérebro e sensor
std::vector<float> sensorOffsets(sensor->rayCount);
for (size_t i = 0; i < sensor->readings.size(); ++i) {
sensorOffsets[i] = sensor->readings[i] ? (1.0f - sensor->readings[i]->offset) : 0.0f;
}
std::vector<float> outputs = NeuralNetwork::feedForward(sensorOffsets, *brain);

if (outputs.size() >= 4) {
// Define intenções de curva baseado na rede, MAS NÃO aplica ainda
controls.forward = (outputs[0] > 0.5f); // Avanço/Ré podem ser aplicados diretamente
intendedTurnLeft = (outputs[1] > 0.5f);
intendedTurnRight = (outputs[2] > 0.5f);
controls.reverse = (outputs[3] > 0.5f);
setControlsFromNN = true; // Marca que a NN tentou definir os controles
}
}

// 4. Aplicar Threshold Anti-Pêndulo (APENAS para IA)
bool overrideTurn = false;
if (setControlsFromNN) { // Apenas se a IA tentou controlar
bool inProblemDistance = (minDistanceAhead >= PENDULUM_DISTANCE_MIN && minDistanceAhead <= PENDULUM_DISTANCE_MAX);
bool isOscillating = ((intendedTurnLeft && lastTurnDirection == 1) || // Tentando ir Esq, vindo da Dir
                 (intendedTurnRight && lastTurnDirection == -1)); // Tentando ir Dir, vindo da Esq

if (inProblemDistance && isOscillating) {
// std::cout << "Threshold activated! Forcing straight." << std::endl; // Debug
controls.left = false;  // Sobrescreve a intenção da IA
controls.right = false; // Sobrescreve a intenção da IA
overrideTurn = true;
} else {
// Aplica a intenção da IA normalmente
controls.left = intendedTurnLeft;
controls.right = intendedTurnRight;
}
}
// Se não for IA ou o threshold não foi ativado, controls.left/right já estão corretos (do teclado ou da IA sem override)


// 5. Atualizar lastTurnDirection (BASEADO NO ESTADO FINAL dos controles)
int currentFinalTurnDirection = 0;
if (controls.left && !controls.right) { // Usa o estado final de controls.left/right
currentFinalTurnDirection = -1;
} else if (controls.right && !controls.left) {
currentFinalTurnDirection = 1;
}
// NÃO atualiza o contador de pêndulo aqui se o override ocorreu,
// pois queremos penalizar a *intenção* de oscilar, mesmo que a tenhamos impedido.
// A lógica de detecção de dano por pêndulo pode ser mantida ou removida.
// Vamos mantê-la por enquanto, mas ela agirá sobre a *intenção* da rede.

// 6. Lógica de Dano por Pêndulo (Opcional - pode ser removida se o threshold funcionar bem)
//    Verifica a *intenção* de oscilar da rede (antes do override)
if (setControlsFromNN && !overrideTurn) { // Só conta se a IA controlou E não houve override
int intendedTurnDirection = 0;
if (intendedTurnLeft && !intendedTurnRight) intendedTurnDirection = -1;
else if (intendedTurnRight && !intendedTurnLeft) intendedTurnDirection = 1;

if ((intendedTurnDirection == -1 && lastTurnDirection == 1) ||
(intendedTurnDirection == 1 && lastTurnDirection == -1)) {
pendulumCounter++;
} else if (intendedTurnDirection == 0) {
pendulumCounter = 0; // Reseta se a IA quis ir reto
}
// else: contador não muda se continuou virando para o mesmo lado

if (pendulumCounter >= 5) {
// std::cout << "DAMAGED by pendulum count!" << std::endl; // Debug
damaged = true;
// Importante: Atualiza lastTurnDirection ANTES de sair se for danificado aqui
lastTurnDirection = currentFinalTurnDirection;
return; // Sai da função update se danificado por contador
}
} else if (!setControlsFromNN || overrideTurn) {
// Reseta contador se o controle não for da IA ou se houve override
pendulumCounter = 0;
}

// Atualiza lastTurnDirection para o próximo frame (se não foi danificado e saiu antes)
lastTurnDirection = currentFinalTurnDirection;

// 7. Mover o Carro
move(); // Move com os controles finais (possivelmente sobrescritos)

// 8. Avaliar Danos por Colisão
// Esta verificação acontece após mover.
damaged = assessDamage(roadBorders, traffic);

} // Fim de Car::update


float Car::calculateMinDistanceAhead(const std::vector<Car*>& traffic, Car** nearestCarOut) {
    float minDistance = std::numeric_limits<float>::max();
    Car* nearest = nullptr;

    for (const auto& trafficCar : traffic) {
         if (trafficCar) {
            // Verifica se o carro de tráfego está À FRENTE
            if (trafficCar->position.y < this->position.y) {
                float distance = std::hypot(
                    this->position.x - trafficCar->position.x,
                    this->position.y - trafficCar->position.y
                );
                if (distance < minDistance) {
                    minDistance = distance;
                    nearest = trafficCar;
                }
            }
         }
    }

    if (nearestCarOut) { // Se um ponteiro de saída foi fornecido
        *nearestCarOut = nearest;
    }

    // Retorna a distância mínima ou um valor grande se nenhum carro estiver à frente
    return (nearest) ? minDistance : std::numeric_limits<float>::max();
}

// move function remains the same
void Car::move() {
    if (controls.forward) { speed += acceleration; }
    if (controls.reverse) { speed -= acceleration; }

    if (speed > maxSpeed) { speed = maxSpeed; }
    if (speed < -maxSpeed / 2.0f) { speed = -maxSpeed / 2.0f; }

    if (std::abs(speed) > friction) {
        if (speed > 0.0f) { speed -= friction; }
        else { speed += friction; }
    } else { speed = 0.0f; }

    if (speed != 0.0f) {
        float flip = (speed > 0.0f) ? 1.0f : -1.0f;
        float turnRate = 0.03f; // * std::abs(speed / maxSpeed); // Optional speed-dependent turn rate
        if (controls.left) { angle -= turnRate * flip; }
        if (controls.right) { angle += turnRate * flip; }
    }

    position.x -= std::sin(angle) * speed;
    position.y -= std::cos(angle) * speed;
}

// getPolygon function remains the same
std::vector<sf::Vector2f> Car::getPolygon() const {
    std::vector<sf::Vector2f> points(4);

    const float rad = std::hypot(width, height) / 2.0f;
    const float alpha = std::atan2(width, height);

    // Calculate offsets explicitly as floats first
    float sin_angle_minus_alpha_rad = static_cast<float>(std::sin(angle - alpha) * rad);
    float cos_angle_minus_alpha_rad = static_cast<float>(std::cos(angle - alpha) * rad);
    float sin_angle_plus_alpha_rad = static_cast<float>(std::sin(angle + alpha) * rad);
    float cos_angle_plus_alpha_rad = static_cast<float>(std::cos(angle + alpha) * rad);

    // Use M_PI defined as float if available, or cast double M_PI
    const float PI_F = static_cast<float>(M_PI); // Define a float PI
    float sin_pi_angle_minus_alpha_rad = static_cast<float>(std::sin(PI_F + angle - alpha) * rad);
    float cos_pi_angle_minus_alpha_rad = static_cast<float>(std::cos(PI_F + angle - alpha) * rad);
    float sin_pi_angle_plus_alpha_rad = static_cast<float>(std::sin(PI_F + angle + alpha) * rad);
    float cos_pi_angle_plus_alpha_rad = static_cast<float>(std::cos(PI_F + angle + alpha) * rad);

    // Now assign using the pre-calculated float offsets
    points[0] = { position.x - sin_angle_minus_alpha_rad, position.y - cos_angle_minus_alpha_rad };
    points[1] = { position.x - sin_angle_plus_alpha_rad,  position.y - cos_angle_plus_alpha_rad };
    points[2] = { position.x - sin_pi_angle_minus_alpha_rad, position.y - cos_pi_angle_minus_alpha_rad };
    points[3] = { position.x - sin_pi_angle_plus_alpha_rad,  position.y - cos_pi_angle_plus_alpha_rad };



    return points;
}

// assessDamage function remains the same
bool Car::assessDamage(const std::vector<std::pair<sf::Vector2f, sf::Vector2f>>& roadBorders,
                       const std::vector<Car*>& traffic)
{
    std::vector<sf::Vector2f> carPoly = getPolygon();
    for (const auto& border : roadBorders) {
         std::vector<sf::Vector2f> borderPoly = {border.first, border.second};
        if (polysIntersect(carPoly, borderPoly)) {
            return true;
        }
    }
    for (const Car* otherCar : traffic) {
         if (otherCar == this || otherCar == nullptr) continue;
        if (polysIntersect(carPoly, otherCar->getPolygon())) {
            return true;
        }
    }
    return false;
}


// Modified draw function
void Car::draw(sf::RenderTarget& target, bool drawSensorFlag) {

    if (textureLoaded) {
        // --- Draw Sprite (if texture loaded) ---
        if (damaged) {
            sprite.setColor(sf::Color(150, 150, 150, 180)); // Gray transparent if damaged
        } else {
            sprite.setColor(color); // Original color if not damaged
        }
        sprite.setPosition(position);
        sprite.setRotation(sf::degrees(-angle * 180.0f / static_cast<float>(M_PI)));
        target.draw(sprite);
    } else {
        // --- Draw Fallback Rectangle (if texture failed to load) ---
        sf::RectangleShape fallbackRect({width, height}); // Size
        fallbackRect.setOrigin({width / 2.0f, height / 2.0f}); // Center origin
        fallbackRect.setPosition(position);
        fallbackRect.setRotation(sf::degrees(-angle * 180.0f / static_cast<float>(M_PI)));

        sf::Color fallbackColor;
        switch (controlType) {
            case ControlType::DUMMY: fallbackColor = sf::Color::Red; break;
            case ControlType::AI:    fallbackColor = sf::Color::Black; break;
            case ControlType::KEYS:  fallbackColor = sf::Color::Blue; break;
            default: fallbackColor = sf::Color::White; break; // Should not happen
        }

        if (damaged) {
            fallbackRect.setFillColor(sf::Color(150, 150, 150, 180)); // Gray transparent if damaged
        } else {
            fallbackRect.setFillColor(fallbackColor);
        }
        target.draw(fallbackRect);
    }

    // --- Draw Sensor (always drawn if requested and available) ---
    if (sensor && drawSensorFlag && !damaged) {
        sensor->draw(target);
    }

     // --- Debug Polygon Drawing (remains the same) ---
     /*
     if (!damaged) {
        std::vector<sf::Vector2f> poly = getPolygon();
        sf::VertexArray lines(sf::LineStrip, poly.size() + 1);
        for(size_t i = 0; i < poly.size(); ++i) {
            lines[i].position = poly[i];
            lines[i].color = sf::Color::Magenta; // Changed color for visibility
        }
        lines[poly.size()] = lines[0];
        target.draw(lines);
     }
     */
}   