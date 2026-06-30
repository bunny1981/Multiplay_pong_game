#include <chrono>
#include <thread>
#include "networking.hpp"
#include "constants.hpp"

int turn = 0;
void reset(ball& gameBall){
    gameBall.positionX = screenWidth/2; 
    gameBall.positionY = screenHeight/2;
    gameBall.velocityX =  (turn == 0 ? 150 : -150);
    gameBall.velocityY = 0;
}

int main(){
    NET::serverSocket server(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    ball gameBall(screenWidth/2, screenHeight/2, -100, 0);
    
    auto lastTime = std::chrono::high_resolution_clock::now();

    std::map<int, std::pair<float, float>> playerData; // playerID -> (currentPos, previousPos)



    while (true){
        auto currentTime = std::chrono::high_resolution_clock::now();
        std::chrono::duration<float> elapsedTime = currentTime - lastTime;
        lastTime = currentTime;

        float deltaT = elapsedTime.count(); // Time in seconds since last loop

        gameBall.positionX += gameBall.velocityX * deltaT;
        gameBall.positionY += gameBall.velocityY * deltaT;

        if (gameBall.positionY > screenHeight - ballRadius || gameBall.positionY < ballRadius){
            gameBall.velocityY = -gameBall.velocityY;
        }

        server.pollNetwork();

        std::vector<int> IDs = server.getIDs();
        if (IDs.size() < 2){
            reset(gameBall);
            continue;
        }
        
        while (server.hasMessages()){
            NetworkMessage message = server.getMessage();

            // Ok so now the server knows who sent what

            int playerID = message.client_fd;
            std::string data = message.payload;

            if (playerData.find(playerID) != playerData.end()){
                playerData[playerID].second = playerData[playerID].first;
                playerData[playerID].first = std::stof(data);
            }
            else{
                playerData[playerID].first = std::stof(data);
                playerData[playerID].second = std::stof(data);
            }

            server.sendMessageExcept(playerID, "P" + data); // Basically tell everyone the position of this player
        }
        
        if (gameBall.positionX + ballRadius >= screenWidth - paddleWidth){
            if (gameBall.positionY + ballRadius/2 < playerData[IDs[1]].first || gameBall.positionY - ballRadius/2 > playerData[IDs[1]].first + paddleLength){
                reset(gameBall);
                turn = 1 - turn;
            }
            else{
                float paddleCenter = (playerData[IDs[1]].first + paddleLength/2.0f);
                float delta = gameBall.positionY - paddleCenter;
                float normalized = delta / (paddleLength / 2.0f);


                gameBall.velocityX = -gameBall.velocityX;
                gameBall.velocityX *= 1.1f;

                gameBall.velocityY = normalized * 100.0f;
                gameBall.velocityY += (playerData[IDs[1]].first - playerData[IDs[1]].second) * 3;
            }
        }
        else if (gameBall.positionX <= ballRadius + paddleWidth){
            if (gameBall.positionY + ballRadius/2 < playerData[IDs[0]].first || gameBall.positionY - ballRadius/2 > playerData[IDs[0]].first + paddleLength){
                reset(gameBall);
                turn = 1 - turn;
            }
            else{
                float paddleCenter = (playerData[IDs[0]].first + paddleLength/2.0f);
                float delta = gameBall.positionY - paddleCenter;
                float normalized = delta / (paddleLength / 2.0f);

                gameBall.velocityX = -gameBall.velocityX;
                gameBall.velocityX *= 1.1f;

                gameBall.velocityY = normalized * 100.0f;
                gameBall.velocityY += (playerData[IDs[0]].first - playerData[IDs[0]].second) * 3;
            }
        }
        
        if (gameBall.velocityX > 1000.0f) gameBall.velocityX = 1000.0f;
        if (gameBall.velocityX < -1000.0f) gameBall.velocityX = -1000.0f;

        std::string ballP1 = "";
        ballP1 += std::to_string(gameBall.positionX);
        ballP1 += '|';
        ballP1 += std::to_string(gameBall.positionY);
        ballP1 += '|';
        ballP1 += std::to_string(gameBall.velocityX);
        ballP1 += '|';
        ballP1 += std::to_string(gameBall.velocityY);

        std::string ballP2 = "";
        ballP2 += std::to_string(screenWidth - gameBall.positionX);
        ballP2 += '|';
        ballP2 += std::to_string(gameBall.positionY);
        ballP2 += '|';
        ballP2 += std::to_string(-gameBall.velocityX);
        ballP2 += '|';
        ballP2 += std::to_string(gameBall.velocityY);

        server.sendMessage(IDs[0], ballP1);
        server.sendMessage(IDs[1], ballP2);

        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
}