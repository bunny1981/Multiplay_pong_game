#include "raylib.h"
#include "rlgl.h"
#include "networking.hpp"
#include "constants.hpp"

int main(){
    NET::clientSocket client(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (client.connectServer("147.185.221.180", 13088) == false){
        return 1;
    }

    InitWindow(screenWidth, screenHeight, "Pong");
    InitAudioDevice();
    SetTargetFPS(60);

    Texture2D ballTex = LoadTexture("assets/ball.png");
    Texture2D stadiumTex = LoadTexture("assets/stadium.png");

    paddle myPaddle(0, screenHeight/2 - paddleLength/2);
    paddle enemyPaddle(screenWidth, screenHeight/2 - paddleLength/2);
    ball gameBall(screenWidth/2, screenHeight/2, -100, 0);

    while (!WindowShouldClose()){
        if (IsKeyDown(KEY_UP)){
            myPaddle.positionY -= 5;
            if (myPaddle.positionY < 0.0f){
                myPaddle.positionY = 0.0f;
            } 
        }
        if (IsKeyDown(KEY_DOWN)){
            myPaddle.positionY += 5;
            if (myPaddle.positionY + paddleLength > screenHeight){
                myPaddle.positionY = screenHeight - paddleLength;
            }
        }

        // Send the paddle and ball data over to the server
        std::string myData = std::to_string(myPaddle.positionY);
        if (client.sendData(myData) == false){
            std::cerr << "Error sending data to server:" << strerror(errno) << std::endl;
        }

        // Recieve the enemy paddle data from the server
        while (true){
            std::string recievedData = client.getData().payload;
            if (recievedData.empty()) break;

            if (recievedData[0] == 'P'){
                // Opponent Data
                std::string pos = recievedData.substr(1);
                enemyPaddle.positionY = std::stof(pos);
            }
            else{
                // Ball Data
                std::vector<std::string> ballInfo;

                std::string data = "";
                for (int i = 0; i < (int)recievedData.size(); i++){
                    if (recievedData[i] == '|'){
                        ballInfo.push_back(data);
                        data.clear();
                    }
                    else{
                        data += recievedData[i];
                    }
                }
                ballInfo.push_back(data);

                gameBall.positionX = std::stof(ballInfo[0]);
                gameBall.positionY = std::stof(ballInfo[1]);
                gameBall.velocityX = std::stof(ballInfo[2]);
                gameBall.velocityY = std::stof(ballInfo[3]);
            }
        }

        BeginDrawing();
        ClearBackground(RAYWHITE);

        Rectangle source = {0.0f, 0.0f, (float)stadiumTex.width, (float)stadiumTex.height};
        Rectangle dest = {0.0f, 0.0f, screenWidth, screenHeight};
        Vector2 origin = {0, 0};
        DrawTexturePro(stadiumTex, source, dest, origin, 0.0f, WHITE);

        DrawRectangle(myPaddle.positionX, myPaddle.positionY, paddleWidth, paddleLength, BLACK);
        DrawRectangle(enemyPaddle.positionX - paddleWidth, enemyPaddle.positionY, paddleWidth, paddleLength, BLACK);

        Rectangle sourceBall = {0.0f, 0.0f, (float)ballTex.width, (float)ballTex.height};
        Rectangle destBall = {gameBall.positionX - ballRadius, gameBall.positionY - ballRadius, 2*ballRadius, 2*ballRadius};
        Vector2 originBall = {0, 0};

        DrawTexturePro(ballTex, sourceBall, destBall, originBall, 0.0f, WHITE);

        EndDrawing();
    }

    UnloadTexture(ballTex);
    UnloadTexture(stadiumTex);

    CloseAudioDevice();
    CloseWindow();
}