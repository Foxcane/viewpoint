#include <SFML/Graphics.hpp>
#include <SFML/System/Vector2.hpp>
#include <cstring>
#include <string>
#include <filesystem>
#include <vector>
#include <iostream>

/* access(), R_OK, F_OK */
#ifdef __linux__
#include <unistd.h>
#define ACCESS access
#endif

/* access(), R_OK, F_OK */
#ifdef _WIN32
#include <io.h>
#define R_OK 4
#define F_OK 0
#define ACCESS _access
#endif

struct PictureData {
    std::string PicName;
    sf::Texture PicTexture;
};

std::vector<PictureData> Pictures;
sf::VideoMode ScreenInfo;

bool isPicture(std::string Filename) {
    bool Result = false;
    std::vector <std::string> SupportedTypes = { ".jpg", ".bmp", ".tga", ".gif", ".psd", ".pic", ".hdr" };
    for (auto Type : SupportedTypes) {
        if (Filename.find(Type) != std::string::npos) {
            Result = true;
            break;
        }
    }
    return Result;
}

void loadPictures(std::string DirPictures) {
    const std::filesystem::path CurrentDir{ DirPictures };
    for (auto const& DirEntry : std::filesystem::directory_iterator{ CurrentDir }) {

#ifdef __linux__
        std::string Filename = DirEntry.path();
#endif

#ifdef _WIN32
        std::string Filename = (DirEntry.path()).string();
#endif

        if (ACCESS(Filename.c_str(), R_OK) == -1) {
            std::cout << Filename << ": permission denied\n";
            continue;
        }
        std::string OriginFilename = Filename;
        int Count = 0;
        for (auto Elem : Filename) { Filename[Count] = std::tolower(Elem); Count++; }
        if (isPicture(Filename)) {
            PictureData Picture;
            Picture.PicName = OriginFilename;
            sf::Texture CurTexture;
            if (CurTexture.loadFromFile(OriginFilename)) { Picture.PicTexture = CurTexture; };
            Pictures.push_back(Picture);
        }
    }
}

float getReductionFactor(sf::Sprite& CurSprite) {
    sf::FloatRect SpriteSize = CurSprite.getGlobalBounds();
    unsigned int DifferenceFromWidths = ScreenInfo.width - static_cast<unsigned int>(SpriteSize.width);
    unsigned int DifferenceFromHeights = ScreenInfo.height - static_cast<unsigned int>(SpriteSize.height);
    float ReductionFactor = 1.0;
    if (DifferenceFromWidths > DifferenceFromHeights) {
        ReductionFactor = static_cast<float>(ScreenInfo.width) / static_cast<float>(SpriteSize.width);
    }
    else {
        ReductionFactor = static_cast<float>(ScreenInfo.height) / static_cast<float>(SpriteSize.height);
    }
    return ReductionFactor;
}

sf::Vector2u resizePicture(sf::Sprite &CurSprite) {
    float ReductionFactor = getReductionFactor(CurSprite);    
    if (ReductionFactor > 1.0) ReductionFactor = 1.0;
    if (ReductionFactor < 1.0) std::cout << "Picture bigger than screen!\n";
    sf::FloatRect CurSpriteSize = CurSprite.getGlobalBounds();
    CurSprite.setScale(ReductionFactor, ReductionFactor);
    sf::Vector2u newSize;
    newSize.x = static_cast<unsigned int>(CurSpriteSize.width * ReductionFactor);
    newSize.y = static_cast<unsigned int>(CurSpriteSize.height * ReductionFactor);
    return newSize;
}

void displayPicture(size_t CurSpriteNum, sf::RenderWindow& Window) {
    sf::Sprite CurSprite(Pictures[CurSpriteNum].PicTexture);
    sf::FloatRect CurSpriteSize = CurSprite.getGlobalBounds();
    unsigned int CurSpriteWidth = CurSpriteSize.width;
    unsigned int CurSpriteHeight = CurSpriteSize.height;

    do {
        sf::Vector2u pictureSize = resizePicture(CurSprite);
        CurSpriteWidth = pictureSize.x;
        CurSpriteHeight = pictureSize.y;
    } while((CurSpriteWidth > ScreenInfo.width) || (CurSpriteHeight > ScreenInfo.height));

    Window.create(sf::VideoMode(CurSpriteWidth, CurSpriteHeight), Pictures[CurSpriteNum].PicName, sf::Style::Close);
    Window.setVisible(false);
    Window.setPosition(sf::Vector2i(sf::VideoMode::getDesktopMode().width / 2 - Window.getSize().x / 2, sf::VideoMode::getDesktopMode().height / 2 - Window.getSize().y / 2));
    Window.setVisible(true);
    Window.draw(CurSprite);
    Window.display();
}

int main(int argc, char* argv[]) {
    std::string CurDir;
    if (argc > 1) {
        if (isPicture(argv[1]) == true) {
            CurDir = argv[1];
            PictureData Picture;
            Picture.PicName = argv[1];
            sf::Texture CurTexture;
            if (CurTexture.loadFromFile(Picture.PicName)) Picture.PicTexture = CurTexture;
            if (errno == 2) return EXIT_FAILURE;
            Pictures.push_back(Picture);
        } else if (isPicture(argv[1]) == false) {
            CurDir = argv[1];
            try {
                std::filesystem::directory_entry PATH {CurDir};
                if (!PATH.exists()) {
                    std::cout << "Directory '" << CurDir << "' is not found.\n";
                    return EXIT_FAILURE;
                }
                loadPictures(CurDir);
                if (Pictures.size() == 0) {
                    std::cout << "Directory '" << CurDir << "' hasn't supported images.";
                    return EXIT_FAILURE;
                }
            } catch(...) {
                std::cout << "Directory access error";
                return EXIT_FAILURE;
            }
        }
    } else {
        CurDir = ".";
        std::filesystem::directory_entry PATH {CurDir};
        if (PATH.exists()) loadPictures(CurDir);
        else { std::cout << "Directory '" << CurDir << "' is not found.\n"; return EXIT_FAILURE; }
        if (Pictures.size() == 0) {
            std::cout << "Directory '" << CurDir << "' hasn't supported images.";
            return EXIT_FAILURE; 
        }
    }
    
    /* get screen size */
    std::cout << Pictures[0].PicName;
    sf::VideoMode Screen;
    ScreenInfo = Screen.getDesktopMode();

    sf::RenderWindow Window;
    std::string CurSpriteName = "";
    size_t CurSpriteNum = 0;

    Window.create(sf::VideoMode(1, 1), "");
    displayPicture(0, Window);

    bool StartMoment = true;
    bool PicReady = false;

    while (Window.isOpen()) {
        sf::Event event;
        if (StartMoment == true) {
            PicReady = true;
            StartMoment = false;
        }
        while (Window.pollEvent(event)) {
            /* close window by mouse */
            if (event.type == sf::Event::Closed) {
                PicReady = false;
                Window.close();
            }
            /* keys pressed handle */
            if (event.type == sf::Event::KeyPressed) {
                /* close window by ESC key */
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::Escape)) {
                    PicReady = false;
                    Window.close();
                }
                if (Pictures.size() > 1) {
                    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right)) {
                        CurSpriteNum++;
                    if (CurSpriteNum > Pictures.size() - 1) CurSpriteNum = 0;
                        PicReady = true; }
                    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left)) {
                        CurSpriteNum = (CurSpriteNum == 0) ? Pictures.size() - 1 : CurSpriteNum - 1;
                        PicReady = true; } 
                }
            }
            if (PicReady == true) {
                displayPicture(CurSpriteNum, Window);
                PicReady = false;
            }
        }
    }
    return EXIT_SUCCESS;
}