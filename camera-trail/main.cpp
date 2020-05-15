#include <escapi.h>
#include <cstdio>
#include <vector>
#include <SFML/Graphics.hpp>

enum class DrawColor
{
	NORMAL,
	RAINBOW,
	GAME_OF_LIFE
};

bool IterateGOL(std::vector<bool>& grid, int rows, int columns)
{
	std::vector<bool> tmp = grid;
	for(int i = 0; i < rows; i++)
	for(int j = 0; j < columns; j++)
	{
		// Count neighbors
		int numNeighbors = 0;
		for(int k = -1; k <= 1; k++)
		for(int l = -1; l <= 1; l++)
		{
			// Wrap around
			int neighborIndex = (i + k) * columns + (j + l);
			if((k == 0 && l == 0) || neighborIndex < 0 || neighborIndex >= rows * columns)
				continue;

			if(grid[neighborIndex])
				numNeighbors++;
		}

		int index = i * columns + j;
		if(grid[index] && (numNeighbors == 2 || numNeighbors == 3))
			continue;
		else if(!grid[index] && numNeighbors == 3)
			tmp[index] = true;
		else
			tmp[index] = false;
	}

	if(tmp != grid)
	{
		grid = tmp;
		return true;
	}
		
	return false;
}

int main()
{
	const int WIDTH = 1280;
	const int HEIGHT = 720;
	const int TRESHOLD = 255;
	bool drawMode = false;	// Drawing or trail

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

	// Game of life grid
	int cellSize = 5;
	int columns = WIDTH / cellSize;
	int rows = HEIGHT / cellSize + 1;
	std::vector<bool> grid;
	grid.resize(rows * columns);

	// Game of life cell
	sf::VertexArray gridVertices(sf::Quads);
	sf::RectangleShape cell(sf::Vector2f(cellSize, cellSize));
	cell.setFillColor(sf::Color(255, 255, 255, 10));
	bool changedGrid = false;

	// Initialize capture for the first device (0)
	if(initCapture(0, &capture) == 0)
	{
		printf("Capture failed - the device may be already in use.\n");
		return -1;
	}

	sf::Image camImage;
	camImage.create(WIDTH, HEIGHT);
	sf::Texture camTexture;

	// Precomputed rainbow colors (for performance)
	std::vector<sf::Color> rainbowColors
	{
		sf::Color::Red,
		sf::Color(255, 50, 0),
		sf::Color(255, 100, 0),
		sf::Color(255, 165, 0),
		sf::Color::Yellow,
		sf::Color(150, 255, 0),
		sf::Color(100, 255, 0),
		sf::Color(50, 255, 0),
		sf::Color::Green,
		sf::Color(0, 255, 50),
		sf::Color(0, 255, 100),
		sf::Color(0, 255, 150),
		sf::Color::Cyan,
		sf::Color(0, 150, 255),
		sf::Color(0, 100, 255),
		sf::Color(0, 50, 255),
		sf::Color::Blue,
		sf::Color(128, 0, 128),
		sf::Color(200, 0, 50)
	};

	DrawColor drawColor = DrawColor::NORMAL;
	unsigned iteration = 0;
	while(window.isOpen())
	{
		sf::Event e;
		while(window.pollEvent(e))
		{
			if(e.type == sf::Event::Closed)
				window.close();

			if(e.type == sf::Event::KeyPressed)
			{
				if(drawMode && e.key.code == sf::Keyboard::Space)
					for(int i = 0; i < HEIGHT; i++)
					for(int j = 0; j < WIDTH; j++)
					{
						const sf::Color& c = camImage.getPixel(j, i);
						camImage.setPixel(j, i, sf::Color(c.r, c.g, c.b, 255));
					}

				if(e.key.code == sf::Keyboard::LControl)
					drawMode = !drawMode;

				if(e.key.code == sf::Keyboard::Num1)
					drawColor = DrawColor::NORMAL;
				else if(e.key.code == sf::Keyboard::Num2)
					drawColor = DrawColor::RAINBOW;
				else if(e.key.code == sf::Keyboard::Num3)
					drawColor = DrawColor::GAME_OF_LIFE;
			}
		}

		// Capture a frame
		doCapture(0);
		
		for(int i = 0; i < HEIGHT; i++)
			for(int j = 0; j < WIDTH; j++)
			{
				const sf::Color& c = camImage.getPixel(j, i);
				int r = (capture.mTargetBuf[i * WIDTH + j] >> 16) & 0xff;
				int g = (capture.mTargetBuf[i * WIDTH + j] >> 8) & 0xff;
				int b =  capture.mTargetBuf[i * WIDTH + j] & 0xff;
				camImage.setPixel(j, i, sf::Color(r, g, b, c.a));

				if(r >= TRESHOLD && g >= TRESHOLD && b >= TRESHOLD)
				{
					camImage.setPixel(j, i, sf::Color(r, g, b) - sf::Color(0, 0, 0, 200));

					int gridIndex = (i / cellSize) * columns + j / cellSize;
					if(!grid.at(gridIndex))
					{
						grid.at(gridIndex) = true;
						changedGrid = true;
					}
				}
				else if(!drawMode && c.a != 255)
					camImage.setPixel(j, i, sf::Color(r, g, b, c.a) + sf::Color(0, 0, 0, 3));
			}

		camTexture.loadFromImage(camImage);
		sf::Sprite camSprite(camTexture);

		if(drawColor == DrawColor::RAINBOW)
		{	
			sf::Color& currentColor = rainbowColors.at(iteration++ % rainbowColors.size());
			window.clear(currentColor);
		} else
			window.clear(sf::Color::White);

		window.draw(camSprite);

		if(drawColor == DrawColor::GAME_OF_LIFE)
		{
			gridVertices.clear();
			for(int i = 0; i < rows; i++)
			for(int j = 0; j < columns; j++)
			{
				if(grid.at(i * columns + j))
				{
					sf::Vector2f pos(j * cellSize, i * cellSize);
					gridVertices.append(sf::Vertex(pos + cell.getPoint(0), sf::Color(255, 255, 255, 100)));
					gridVertices.append(sf::Vertex(pos + cell.getPoint(1), sf::Color(255, 255, 255, 100)));
					gridVertices.append(sf::Vertex(pos + cell.getPoint(2), sf::Color(255, 255, 255, 100)));
					gridVertices.append(sf::Vertex(pos + cell.getPoint(3), sf::Color(255, 255, 255, 100)));
				}
			}

			window.draw(gridVertices);
			IterateGOL(grid, rows, columns);
		}
		window.display();
	}

	while(isCaptureDone(0) == 0) {} // Wait for last capture to end
	deinitCapture(0);
	delete[] capture.mTargetBuf;

	return 0;
}