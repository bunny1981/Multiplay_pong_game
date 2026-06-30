#ifndef CONSTANTS
#define CONSTANTS

struct ball{
    float positionX;
    float positionY;

    float velocityX;
    float velocityY;

    ball(float x, float y, float vX, float vY){
        positionX = x;
        positionY = y;

        velocityX = vX;
        velocityY = vY;
    }
};

struct paddle{
    float positionX;
    float positionY;

    paddle(float x, float y){
        positionX = x;
        positionY = y;
    }
};

inline constexpr const int screenWidth = 600;
inline constexpr const int screenHeight = 800;
inline constexpr const int paddleLength = 90;
inline constexpr const int paddleWidth = 12;
inline constexpr const float ballRadius = 14.0f;

#endif