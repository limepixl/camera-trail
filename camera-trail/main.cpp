#include "thirdparty/escapi3/escapi.h"
#include <cstdio>
#include <SFML/Graphics.hpp>

int main()
{
	const int WIDTH = 1280;
	const int HEIGHT = 720;
	const int TRESHOLD = 250;

	// Create SFML window
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
	capture.mTargetBuf = new int[WIDTH * HEIGHT];

	// Initialize capture for the first device (0)
	if(initCapture(0, &capture) == 0)
	{
		printf("Capture failed - the device may be already in use.\n");
		return -1;
	}

	sf::Image camImage;
	camImage.create(WIDTH, HEIGHT);
	sf::Texture camTexture;

	sf::Image lightMap;
	lightMap.create(WIDTH, HEIGHT, sf::Color(255, 255, 255, 0));
	sf::Texture lightMapTexture;

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
				camImage.setPixel(j, i, sf::Color(r, g, b));

				if(r >= TRESHOLD && g >= TRESHOLD && b >= TRESHOLD)
					lightMap.setPixel(j, i, sf::Color::White);
				else
					lightMap.setPixel(j, i, lightMap.getPixel(j, i) - sf::Color(0, 0, 0, 10));
			}
		}

		camTexture.loadFromImage(camImage);
		sf::Sprite camSprite(camTexture);

		lightMapTexture.loadFromImage(lightMap);
		sf::Sprite lightMapSprite(lightMapTexture);

		window.clear(sf::Color::Black);
		window.draw(camSprite);
		window.draw(lightMapSprite);
		window.display();
	}

	delete[] capture.mTargetBuf;
	deinitCapture(0);
	return 0;
}