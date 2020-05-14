#include "thirdparty/escapi3/escapi.h"
#include <cstdio>
#include <SFML/Graphics.hpp>

int main()
{
	// Create SFML window
	const int WIDTH = 1280;
	const int HEIGHT = 720;
	sf::RenderWindow window(sf::VideoMode(WIDTH, HEIGHT), "Camera Trail (Stefan Ivanovski)");
	window.setFramerateLimit(60);

	// Initialize ESCAPI 
	int devices = setupESCAPI();
	if(devices == 0)
	{
		printf("No camera detected!\n");
		return -1;
	}

	// Capture parameters
	SimpleCapParams capture;
	capture.mWidth = WIDTH;
	capture.mHeight = HEIGHT;
	capture.mTargetBuf = new int[WIDTH * HEIGHT * 3];

	// Initialize capture for the first device (0)
	if(initCapture(0, &capture) == 0)
	{
		printf("Capture failed - the device may be already in use.\n");
		return -1;
	}

	sf::Image camImage;
	camImage.create(WIDTH, HEIGHT);

	while(window.isOpen())
	{
		sf::Event e;
		while(window.pollEvent(e))
		{
			if(e.type == sf::Event::Closed)
				window.close();
		}

		doCapture(0);
		while(isCaptureDone(0) == 0)
		{}

		for(int i = 0; i < HEIGHT; i++)
		{
			for(int j = 0; j < WIDTH; j++)
			{
				int r = (capture.mTargetBuf[i * WIDTH + j] >> 16) & 0xff;
				int g = (capture.mTargetBuf[i * WIDTH + j] >> 8) & 0xff;
				int b = capture.mTargetBuf[i * WIDTH + j] & 0xff;
				camImage.setPixel(j, i, sf::Color(r, g, b, 255));
			}
		}

		sf::Texture camTexture;
		camTexture.loadFromImage(camImage);

		sf::Sprite camSprite(camTexture);

		window.clear(sf::Color::Black);
		window.draw(camSprite);
		window.display();
	}

	deinitCapture(0);
	return 0;
}