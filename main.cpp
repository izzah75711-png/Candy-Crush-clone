#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <ctime>      // For time
#include <cmath>
#include <sstream>

using namespace std;

//GLOBAL ENUMS

enum GameState {
    STATE_INTRO,
    STATE_MENU,
    STATE_SETTINGS,
    STATE_LEVELS,
    STATE_LEVEL1,
    STATE_LEVEL_END_PREVIEW,
    STATE_WIN,
    STATE_FAIL,
    STATE_ACHIEVEMENT,
    STATE_LIVES_WAITING,
    STATE_INSTRUCTIONS,
    STATE_PAUSED
};

// State machine for handling match-3 sequence animations
enum GameLogicState {
    LOGIC_IDLE, LOGIC_SWAPPING, LOGIC_CHECKING, LOGIC_FALLING, LOGIC_REFILLING, LOGIC_BOOSTER_EXPLODING
};

//candy types
enum CandyType { TYPE_1, TYPE_2, TYPE_3, TYPE_4, TYPE_5, TYPE_WRAPPED, TYPE_EMPTY = 99 };

int maximum(int a, int b) {
    return (a > b) ? a : b;
}

float maximum(float a, float b) {
    return (a > b) ? a : b;
}

//function to swap candies
void swap(CandyType& a, CandyType& b) {
    CandyType temp = a;
    a = b;
    b = temp;
}

// SFML State Variables
GameState gameState = STATE_INTRO;
GameLogicState logicState = LOGIC_IDLE;
const int MAX_GRID_SIZE = 9;
GameState previousGameState = STATE_MENU;

CandyType level1Candies[MAX_GRID_SIZE][MAX_GRID_SIZE];
int matched[MAX_GRID_SIZE][MAX_GRID_SIZE];

sf::Texture candyTextures[6];
sf::Font mainFont;

// Sound variables
sf::SoundBuffer successSoundBuffer1, successSoundBuffer2;
sf::SoundBuffer failSoundBuffer1, failSoundBuffer2;
sf::SoundBuffer boosterSoundBuffer1, boosterSoundBuffer2, boosterSoundBuffer3;

sf::Sound successSound1, successSound2;
sf::Sound failSound1, failSound2;
sf::Sound boosterSound1, boosterSound2, boosterSound3;

sf::Music backgroundMusic;
sf::Music levelStateMusic;


// Game Logic Variables (These are saved/loaded)
int currentLevel = 1;
int maxUnlockedLevel = 1;
int lives = 3;
int volume = 5;
bool isMuted = false;
int totalStars = 0;
// CHANGED ARRAY SIZE to MAX_LEVEL_COUNT (10) for consistency
const int MAX_LEVEL_COUNT = 10;
int levelStars[MAX_LEVEL_COUNT] = { 0 };

long long livesEndTimestamp = 0;

// Game Logic Variables
int currentlivestars = 0;
int score = 0;
int currentMoves = 0;
int selectedR = -1; int selectedC = -1;
int secondR = -1; int secondC = -1;
bool swapRequested = false;
int boosterR = -1;
int boosterC = -1;
int boosterUseCount = 0;
bool failSound2Played = false;

// HIGH SCORES ARRAY KEPT
long long highScores[MAX_LEVEL_COUNT] = { 0 };


// Booster Explosion Variables
sf::Clock boosterExplosionClock;
const float BOOSTER_EXPLOSION_DURATION = 0.5f;
int boosterExplodeR = -1;
int boosterExplodeC = -1;


sf::Clock transitionClock;
sf::Clock endLevelClock;

sf::Clock levelEndPreviewClock;
GameState nextLevelState = STATE_LEVELS;

// CONSTANT
const float LIVES_REFILL_TIME = 60.0f;
const float LEVEL_END_PREVIEW_DURATION = 3.0f;
const float INTRO_DISPLAY_TIME = 3.0f;
const float FADE_OUT_TIME = 0.5f;
const float LOADING_SHOW_TIME = 3.0f;
const float END_LEVEL_SCREEN_DURATION = 6.0f;
float volumeValue = 0.5f;
float previousVolumeValue = volumeValue;
bool volumeEnabled = true;

// Score thresholds per level
int scorethreshold[10][3] = {
    {1000,5000,10000},    // LEVEL 1 (index 0)
    {1000,5000,10000},    // LEVEL 2 (index 1)
    {2000,6000,12000},    // LEVEL 3 (index 2)
    {2000,6000,12000},    // LEVEL 4 (index 3)
    {3000,7000,14000},    // LEVEL 5 (index 4)
    {3000,7000,14000},    // LEVEL 6 (index 5)
    {4000,8000,16000},    // LEVEL 7 (index 6)
    {4000,8000,16000},    // LEVEL 8 (index 7)
    {5000,10000,18000},    // LEVEL 9 (index 8)
    {5000,10000,18000} // LEVEL 10 (index 9)
};


//FORWARD DECLARATIONS
void saveGameProgress();
void calculateTotalStars();
void resetGame();
int calculateStars(int score, int currentLevel);
void updateStars();
bool isLevelPassedForWin();
void failLevel();
void triggerFailure();
void quitLevel();
void completeLevel();
void playLevel(int gain);
bool CheckForPossibleMoves(int gridSize);
void ShuffleBoard(int gridSize);
void refillBoard(int gridSize);
void updateHighScores(int level, long long currentScore);

//END FORWARD DECLARATIONS
void updateHighScores(int level, long long currentScore) {
    if (level >= 1 && level <= MAX_LEVEL_COUNT) {
        int index = level - 1;

        if (currentScore > highScores[index]) {
            highScores[index] = currentScore;

            saveGameProgress();
            cout << "New High Score for Level " << level << ": " << currentScore << endl;
        }
    }
}
void resetGame() {
    maxUnlockedLevel = 1;
    lives = 3;
    totalStars = 0;
    livesEndTimestamp = 0;
    // Use MAX_LEVEL_COUNT
    for (int i = 0; i < MAX_LEVEL_COUNT; ++i) {
        levelStars[i] = 0;
        highScores[i] = 0; // Reset high scores too
    }
    gameState = STATE_MENU;
    saveGameProgress();
    cout << "Game progress reset. Returning to menu." << endl;
}

void pauseGame() {
    if (gameState == STATE_LEVEL1) {
        previousGameState = gameState; // current state save karein
        gameState = STATE_PAUSED;
        levelStateMusic.pause(); // Level music pause karein
        cout << "Game paused!" << endl;
    }
}

void resumeGame() {
    if (gameState == STATE_PAUSED) {
        gameState = previousGameState; // saved state restore karein
        levelStateMusic.play(); // Level music resume karein
        cout << "Game resumed!" << endl;
    }
}


void calculateTotalStars() {
    totalStars = 0;
    // Use MAX_LEVEL_COUNT
    for (int i = 0; i < MAX_LEVEL_COUNT; ++i) {
        totalStars += levelStars[i];
    }
}
// ------------------- Default settings (added) -------------------
void setdefaultsettings() {
    maxUnlockedLevel = 1;
    lives = 3;
    volume = 5;
    isMuted = false;

    volumeEnabled = true;
    volumeValue = 0.5f;

    livesEndTimestamp = 0;

    // Use MAX_LEVEL_COUNT
    for (int i = 0; i < MAX_LEVEL_COUNT; i++) {
        levelStars[i] = 0;
        highScores[i] = 0; // Initialize high scores
    }

    calculateTotalStars();
    // Save defaults to ensure a valid savegame file exists
    saveGameProgress();

    cout << "Default settings loaded.\n";
}


void loadGameProgress() {
    ifstream infile("savegame.txt");

    if (!infile.is_open()) {
        cout << "Save file not found! Using default progress." << endl;
        setdefaultsettings();
        return;
    }

    // --- 1. Read the 5 main global variables (all int) ---
    int file_maxlevel, file_lives, file_volume, file_mute;
    long long file_timestamp; // Use long long for timestamp

    if (!(infile >> file_maxlevel >> file_lives >> file_volume >> file_mute >> file_timestamp)) {
        infile.close();
        cout << "Corrupted save: failed to read main global values. Using default progress." << endl;
        setdefaultsettings();
        return;
    }

    int file_stars[MAX_LEVEL_COUNT];
    for (int i = 0; i < MAX_LEVEL_COUNT; ++i) {
        if (!(infile >> file_stars[i])) {
            infile.close();
            cout << "Corrupted save: failed to read all star values. Using default progress." << endl;
            setdefaultsettings();
            return;
        }
    }

    long long file_highScores[MAX_LEVEL_COUNT];
    for (int i = 0; i < MAX_LEVEL_COUNT; ++i) {
        if (!(infile >> file_highScores[i])) {
            infile.close();
            cout << "Corrupted save: failed to read all high score values. Using default progress." << endl;
            setdefaultsettings();
            return;
        }
    }

    infile.close(); // Close file after successful read

    // ---- VALIDATE AND ASSIGN MAIN VALUES ----
    if (file_maxlevel < 1 || file_maxlevel > 10 || file_lives < 0 || file_lives > 5 ||
        file_volume < 0 || file_volume > 10 || (file_mute != 0 && file_mute != 1)) {
        cout << "Corrupted save: invalid main values. Using default progress." << endl;
        setdefaultsettings();
        return;
    }

    // ---- VALIDATE STARS ----
    for (int i = 0; i < MAX_LEVEL_COUNT; i++) {
        if (file_stars[i] < 0 || file_stars[i] > 3) {
            cout << "Corrupted save: invalid star values. Using default progress." << endl;
            setdefaultsettings();
            return;
        }
    }


    for (int i = 0; i < MAX_LEVEL_COUNT; i++) {
        if (file_highScores[i] < 0) {
            file_highScores[i] = 0;
        }
    }

    // ---- ASSIGN GLOBAL VALUES SAFELY ----
    lives = file_lives;
    volume = file_volume;
    volumeEnabled = (file_mute == 0);
    volumeValue = volumeEnabled ? (float)file_volume / 10.f : 0.f;
    livesEndTimestamp = file_timestamp;


    for (int i = 0; i < MAX_LEVEL_COUNT; i++) {
        levelStars[i] = file_stars[i];
        highScores[i] = file_highScores[i];
    }

    int revalidatedMaxLevel = 1;
    for (int i = 0; i < MAX_LEVEL_COUNT; ++i) {
        // If the current level (i+1) has 1 or more stars
        if (levelStars[i] >= 1) {
            // Unlock the next level (i+2)
            revalidatedMaxLevel = i + 2;
        }
        else {
            // If Level i+1 has 0 stars, stop here and lock subsequent levels.
            break;
        }
    }
    if (revalidatedMaxLevel > MAX_LEVEL_COUNT) revalidatedMaxLevel = MAX_LEVEL_COUNT;

    // Overwrite the file's potentially incorrect maxUnlockedLevel with the revalidated value
    maxUnlockedLevel = revalidatedMaxLevel;
    // >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

    calculateTotalStars();
    cout << "Game progress loaded successfully!" << endl;
}


void saveGameProgress() {
    ofstream outfile("savegame.txt");

    if (!outfile.is_open()) {
        cout << "Oops! Could not save your progress!" << endl;
        return;
    }

    // ----- VALIDATION -----
    if (maxUnlockedLevel < 1) maxUnlockedLevel = 1;
    if (maxUnlockedLevel > 10) maxUnlockedLevel = 10;

    if (lives < 0) lives = 0;
    if (lives > 5) lives = 5; // adjust if your max lives differ

    if (volume < 0) volume = 0;
    if (volume > 10) volume = 10;

    int voltosave = (int)(volumeValue * 10.f + 0.5f);
    if (voltosave < 0) voltosave = 0;
    if (voltosave > 10) voltosave = 10;

    int mutetosave = volumeEnabled ? 0 : 1;
    if (mutetosave != 0 && mutetosave != 1) mutetosave = 0;

    // Fix invalid stars
    for (int i = 0; i < MAX_LEVEL_COUNT; i++) {
        if (levelStars[i] < 0) levelStars[i] = 0;
        if (levelStars[i] > 3) levelStars[i] = 3;

        // Fix invalid high scores
        if (highScores[i] < 0) highScores[i] = 0;
    }

    // ----- WRITE VALUES (5 Globals) -----
    outfile << maxUnlockedLevel << " ";
    outfile << lives << " ";
    outfile << voltosave << " ";
    outfile << mutetosave << " ";
    outfile << livesEndTimestamp << " ";

    // ----- WRITE LEVEL STARS (10 values) -----
    for (int i = 0; i < MAX_LEVEL_COUNT; ++i) {
        outfile << levelStars[i];
        outfile << " "; // Always add a space for separation
    }

    // ----- WRITE HIGH SCORES (10 values) -----
    for (int i = 0; i < MAX_LEVEL_COUNT; ++i) {
        outfile << highScores[i];
        if (i < MAX_LEVEL_COUNT - 1) outfile << " "; // Space only between scores
    }

    outfile << "\n";

    cout << "Your candy kingdom is safe! Progress saved!" << endl;
    outfile.close();
}


//GAME LOGIC FUNCTIONS (Rest of the functions are unchanged from your file)

int getGridSize(int level) {

    if (level == 1 || level == 2) return 5;
    if (level == 3 || level == 4) return 6;
    if (level == 5 || level == 6) return 7;
    if (level == 7 || level == 8) return 8;
    if (level == 9 || level == 10) return 9;
    return 0;
}

int getLevelMoves(int level)
{

    if (level == 1 || level == 2) return 20;
    if (level == 3 || level == 4) return 18;
    if (level == 5 || level == 6) return 15;
    if (level == 7 || level == 8) return 12;
    if (level == 9 || level == 10) return 10;
    return 0;
}

int calculateStars(int score, int currentLevel) {

    if (currentLevel < 1 || currentLevel > 10) return 0;
    int index = currentLevel - 1;
    int first = scorethreshold[index][0];
    int second = scorethreshold[index][1];
    int third = scorethreshold[index][2];

    if (score >= third) return 3;
    else if (score >= second) return 2;
    else if (score >= first) return 1;
    else return 0;
}

void updateStars() {

    currentlivestars = calculateStars(score, currentLevel);
}

bool isLevelPassedForWin() {

    if (currentLevel < 1 || currentLevel > 10) return false;
    int threeStarScore = scorethreshold[currentLevel - 1][2];
    return score >= threeStarScore;
}

void failLevel() {

    if (lives > 0) {
        lives--;
        saveGameProgress();
    }
}

//Handles sound and state transition for a level failure (0 stars or quit)

void triggerFailure() {

    failLevel();

    levelStateMusic.stop();
    failSound2Played = false;

    if (lives <= 0) {
        livesEndTimestamp = time(NULL);
        saveGameProgress();
        gameState = STATE_LIVES_WAITING;
    }
    else {
        failSound1.play();
        gameState = STATE_FAIL;
        endLevelClock.restart();
    }
}


//Handles quitting the level, which is always a fail condition

void quitLevel() {

    if (gameState == STATE_LEVEL1) {
        triggerFailure();
    }
}

// Handles the logic when a level is definitively completed (1+ stars)
void completeLevel() {
    int stars = calculateStars(score, currentLevel);

    int oldStars = levelStars[currentLevel - 1];
    levelStars[currentLevel - 1] = maximum(oldStars, stars);
    calculateTotalStars();

    bool unlockedNewLevel = false;
    if (currentLevel < 10) {

        if (stars >= 1) {
            int nextLevel = currentLevel + 1;

            if (nextLevel > maxUnlockedLevel) {
                maxUnlockedLevel = nextLevel;
                unlockedNewLevel = true;
            }
        }
    }
    updateHighScores(currentLevel, score);
    saveGameProgress();

    levelStateMusic.stop();

    boosterSound1.stop();
    boosterSound2.stop();
    boosterSound3.stop();

    successSound1.play();
    successSound2.play();

    if (unlockedNewLevel) {
        nextLevelState = STATE_ACHIEVEMENT;
    }
    else {
        nextLevelState = STATE_WIN;
    }

    gameState = STATE_LEVEL_END_PREVIEW;
    levelEndPreviewClock.restart();
    endLevelClock.restart();
}

//Manages the score updates and level completion/failure logic
void playLevel(int gain) {

    if (currentMoves == 0 && score == 0) {
        currentMoves = getLevelMoves(currentLevel);
    }

    if (gain > 0) {
        score += gain;
    }

    updateStars();

    if (isLevelPassedForWin()) {
        completeLevel();
        return;
    }

    if (currentMoves <= 0) {
        int stars = calculateStars(score, currentLevel);

        if (stars >= 1) {
            completeLevel();
            return;
        }

        triggerFailure();
        return;
    }
}


//CORE MATCH-3 LOGIC

//Takes CandyType array
void GenerateCandyGrid(CandyType grid[][MAX_GRID_SIZE], int gridSize, int candyTypes)
{

    for (int i = 0; i < MAX_GRID_SIZE; i++) {
        for (int j = 0; j < MAX_GRID_SIZE; j++) {
            //Access the enum
            grid[i][j] = TYPE_EMPTY;
        }
    }

    for (int i = 0; i < gridSize; i++)
    {
        for (int j = 0; j < gridSize; j++)
        {
            if (i >= MAX_GRID_SIZE || j >= MAX_GRID_SIZE) continue;

            do
            {
                //Access the enum
                grid[i][j] = (CandyType)(rand() % 5);

            } while (
                (j >= 2 &&
                    //Check enums
                    grid[i][j] == grid[i][j - 1] &&
                    grid[i][j] == grid[i][j - 2]) ||

                (i >= 2 &&
                    //Check enums
                    grid[i][j] == grid[i - 1][j] &&
                    grid[i][j] == grid[i - 2][j])
                );
        }
    }
}
// restarts the level by resetting variables and generating a new board
void restartLevel(int level) {
    currentLevel = level;
    score = 0;// score set to zero
    currentMoves = getLevelMoves(currentLevel);// set moves based on level
    currentlivestars = 0;// live star count is set to zero
    logicState = LOGIC_IDLE;// no swapping or booster exploding

    selectedR = selectedC = secondR = secondR = -1;// no selection
    swapRequested = false;// no swap possible
    boosterExplodeR = -1;// no booster explosion in row
    boosterExplodeC = -1;// no nooster explosion in column
    boosterUseCount = 0; // reset booster use count to zero
    failSound2Played = false;

    int gridSize = getGridSize(currentLevel);// will get size of new grid on basis of level
    int candyTypes = 5;
    do {
        //Passed the array of enums
        GenerateCandyGrid(level1Candies, gridSize, candyTypes);
    } while (!CheckForPossibleMoves(gridSize));
}


int countMatches(int gridSize)
{
    int matchCount = 0;
    boosterR = -1; boosterC = -1;

    for (int i = 0; i < gridSize; i++) {
        for (int j = 0; j < gridSize; j++) {
            matched[i][j] = 0;
        }
    }

    int tempMatched[MAX_GRID_SIZE][MAX_GRID_SIZE] = { 0 };

    // 1. Horizontal matches
    for (int i = 0; i < gridSize; i++) {
        int count = 1;
        for (int j = 1; j < gridSize; j++) {
            //Access the enum
            CandyType currentType = level1Candies[i][j];
            CandyType previousType = level1Candies[i][j - 1];

            if (currentType >= TYPE_1 && currentType <= TYPE_5 &&
                currentType == previousType) {
                count++;
            }
            else {
                if (count >= 3) {
                    for (int k = 0; k < count; k++) {
                        tempMatched[i][j - 1 - k] = maximum(tempMatched[i][j - 1 - k], count);
                    }
                }
                count = 1;
            }
        }
        if (count >= 3) {
            for (int k = 0; k < count; k++) {
                tempMatched[i][gridSize - 1 - k] = maximum(tempMatched[i][gridSize - 1 - k], count);
            }
        }
    }

    // 2. Vertical matches
    for (int j = 0; j < gridSize; j++) {
        int count = 1;
        for (int i = 1; i < gridSize; i++) {
            //Access the enum
            CandyType currentType = level1Candies[i][j];
            CandyType previousType = level1Candies[i - 1][j];

            if (currentType >= TYPE_1 && currentType <= TYPE_5 &&
                currentType == previousType) {
                count++;
            }
            else {
                if (count >= 3) {
                    for (int k = 0; k < count; k++) {
                        tempMatched[i - 1 - k][j] = maximum(tempMatched[i - 1 - k][j], count);
                    }
                }
                count = 1;
            }
        }
        if (count >= 3) {
            for (int k = 0; k < count; k++) {
                tempMatched[gridSize - 1 - k][j] = maximum(tempMatched[gridSize - 1 - k][j], count);
            }
        }
    }

    // 3. Final Marking & Booster Placement
    for (int i = 0; i < gridSize; i++) {
        for (int j = 0; j < gridSize; j++) {

            if (tempMatched[i][j] >= 3) {
                matched[i][j] = 1;

                if (tempMatched[i][j] >= 4 && boosterR == -1) {
                    if ((i == selectedR && j == selectedC) || (i == secondR && j == secondC)) {
                        boosterR = i; boosterC = j;
                    }
                }
            }
        }
    }

    if (boosterR == -1) {
        for (int i = 0; i < gridSize; i++) {
            for (int j = 0; j < gridSize; j++) {
                if (tempMatched[i][j] >= 4) {
                    boosterR = i; boosterC = j;
                    goto end_booster_search;
                }
            }
        }
    }
end_booster_search:;

    matchCount = 0;
    for (int i = 0; i < gridSize; i++) {
        for (int j = 0; j < gridSize; j++) {
            if (matched[i][j] == 1) {
                matchCount++;
            }
        }
    }

    return matchCount;
}
//Removes matched candies and creates a wrapped booster if applicable.
void removeMatches(int gridSize)
{

    bool boosterCreatedNow = (boosterR != -1 && boosterC != -1);

    if (boosterCreatedNow && gameState == STATE_LEVEL1) {
        boosterSound1.play();
    }

    for (int i = 0; i < gridSize; i++) {
        for (int j = 0; j < gridSize; j++) {
            if (matched[i][j] == 1) {
                //Set enum
                level1Candies[i][j] = TYPE_EMPTY;
            }
        }
    }

    if (boosterCreatedNow) {
        //Set enum
        level1Candies[boosterR][boosterC] = TYPE_WRAPPED;
        boosterR = -1;
        boosterC = -1;
    }
}

void applyGravity(int gridSize)
{

    for (int column = 0; column < gridSize; column++)
    {
        for (int row = gridSize - 1; row >= 0; row--)
        {
            //Check enum
            if (level1Candies[row][column] == TYPE_EMPTY)
            {
                int k = row - 1;
                while (k >= 0 && level1Candies[k][column] == TYPE_EMPTY) //Check enum
                {
                    k--;
                }
                if (k >= 0)
                {
                    //Assign enums
                    level1Candies[row][column] = level1Candies[k][column];
                    level1Candies[k][column] = TYPE_EMPTY;
                }
            }
        }
    }
}


//Refills empty spaces with random candies
void refillBoard(int gridSize)
{

    for (int i = 0; i < gridSize; i++)
    {
        for (int j = 0; j < gridSize; j++)
        {
            //Check enum
            if (level1Candies[i][j] == TYPE_EMPTY)
            {
                //Set enum
                level1Candies[i][j] = (CandyType)(rand() % 5);
            }
        }
    }
}

//Performs the actual candy clearing and score update for the wrapped booster
void clearWrappedEffect(int r, int c, int gridSize) {

    int clearedCount = 0;

    // Clear Row
    for (int j = 0; j < gridSize; j++) {
        //Check enum
        if (level1Candies[r][j] != TYPE_EMPTY) {
            level1Candies[r][j] = TYPE_EMPTY; //Set enum
            clearedCount++;
            playLevel(50);
        }
    }

    // Clear Column
    for (int i = 0; i < gridSize; i++) {
        if (i == r) continue;

        //Check enum
        if (level1Candies[i][c] != TYPE_EMPTY) {
            level1Candies[i][c] = TYPE_EMPTY; //Set enum
            clearedCount++;
            playLevel(50);
        }
    }

    if (clearedCount > 0) {
        playLevel(500);
    }
}

void SwapCandies(int r1, int c1, int r2, int c2)
{

    int gridSize = getGridSize(currentLevel);
    bool activated = false;

    //Check enum
    if (level1Candies[r1][c1] == TYPE_WRAPPED) {
        boosterExplodeR = r1; boosterExplodeC = c1;
        activated = true;
    }
    //Check enum
    else if (level1Candies[r2][c2] == TYPE_WRAPPED) {
        boosterExplodeR = r2; boosterExplodeC = c2;
        activated = true;
    }

    if (activated) {
        if (logicState == LOGIC_SWAPPING) {
            currentMoves--;
        }

        logicState = LOGIC_BOOSTER_EXPLODING;
        boosterExplosionClock.restart();

        boosterUseCount = (boosterUseCount % 3) + 1;
        if (boosterUseCount == 1) boosterSound1.play();
        else if (boosterUseCount == 2) boosterSound2.play();
        else if (boosterUseCount == 3) boosterSound3.play();


        selectedR = -1; selectedC = -1; secondR = -1; secondC = -1;
        swapRequested = false;

        //Set enum
        level1Candies[boosterExplodeR][boosterExplodeC] = TYPE_EMPTY;

        return;
    }

    // Normal swap logic (swap the enums)
    swap(level1Candies[r1][c1], level1Candies[r2][c2]);
}

bool areAdjacent(int r1, int c1, int r2, int c2) {

    return (std::abs(r1 - r2) + std::abs(c1 - c2) == 1);
}

sf::Vector2f getCandyPosition(int r, int c, int gridSize, float cellSize, float gap) {

    const float GAME_AREA_X = 500.f;
    const float GAME_AREA_Y = 200.f;
    const float GAME_AREA_WIDTH = 800.f;
    const float GAME_AREA_HEIGHT = 900.f;

    float cellUnit = cellSize + gap;

    float totalContentWidth = gridSize * cellSize + (gridSize - 1) * gap;
    float totalContentHeight = gridSize * cellSize + (gridSize - 1) * gap;

    float startX = GAME_AREA_X + (GAME_AREA_WIDTH - totalContentWidth) / 2.f;
    float startY = GAME_AREA_Y + (GAME_AREA_HEIGHT - totalContentHeight) / 2.f;

    return sf::Vector2f(startX + c * cellUnit, startY + r * cellUnit);
}


//NEW BOARD MANAGEMENT FUNCTIONS

//Checks the entire grid for any swap that would create a match of 3 or more
bool CheckForPossibleMoves(int gridSize) {

    int originalSelectedR = selectedR;
    int originalSelectedC = selectedC;
    int originalSecondR = secondR;
    int originalSecondC = secondC;

    for (int r = 0; r < gridSize; ++r) {
        for (int c = 0; c < gridSize; ++c) {

            // Try swapping with the right neighbor
            if (c + 1 < gridSize) {
                swap(level1Candies[r][c], level1Candies[r][c + 1]);

                selectedR = r; selectedC = c;
                secondR = r; secondC = c + 1;

                if (countMatches(gridSize) > 0) {

                    swap(level1Candies[r][c], level1Candies[r][c + 1]);

                    selectedR = originalSelectedR; selectedC = originalSelectedC;
                    secondR = originalSecondR; secondC = originalSecondC;

                    return true;
                }
                swap(level1Candies[r][c], level1Candies[r][c + 1]);
            }

            // Try swapping with the bottom neighbor
            if (r + 1 < gridSize) {
                swap(level1Candies[r][c], level1Candies[r + 1][c]);

                selectedR = r; selectedC = c;
                secondR = r + 1; secondC = c;

                if (countMatches(gridSize) > 0) {
                    swap(level1Candies[r][c], level1Candies[r + 1][c]);

                    selectedR = originalSelectedR; selectedC = originalSelectedC;
                    secondR = originalSecondR; secondC = originalSecondC;

                    return true;
                }
                swap(level1Candies[r][c], level1Candies[r + 1][c]);
            }
        }
    }

    selectedR = originalSelectedR; selectedC = originalSelectedC;
    secondR = originalSecondR; secondC = originalSecondC;

    return false;
}

//Shuffles the board until a valid move is found
void ShuffleBoard(int gridSize) {

    CandyType tempTypes[MAX_GRID_SIZE * MAX_GRID_SIZE];
    int tempIndex = 0;

    // 1. Collect all existing normal candy types
    for (int i = 0; i < gridSize; ++i) {
        for (int j = 0; j < gridSize; ++j) {
            //Check enum
            if (level1Candies[i][j] >= TYPE_1 && level1Candies[i][j] <= TYPE_5) {
                tempTypes[tempIndex++] = level1Candies[i][j];
            }
        }
    }

    int shufflableCount = tempIndex;

    bool moveFound = false;
    while (!moveFound) {

        // 2. Randomly shuffle the collected candy types
        for (int i = shufflableCount - 1; i > 0; --i) {
            int j = std::rand() % (i + 1);
            //Use the new custom swap function
            swap(tempTypes[i], tempTypes[j]);
        }

        // 3. Place shuffled candies back onto the board
        tempIndex = 0;
        for (int i = 0; i < gridSize; ++i) {
            for (int j = 0; j < gridSize; ++j) {
                //Check enum
                if (level1Candies[i][j] >= TYPE_1 && level1Candies[i][j] <= TYPE_5) {
                    //Assign enum
                    level1Candies[i][j] = tempTypes[tempIndex++];
                }
            }
        }

        // 4. Check for and resolve immediate matches on the new board
        int initialMatchCount = countMatches(gridSize);
        if (initialMatchCount > 0) {
            do {
                removeMatches(gridSize);
                applyGravity(gridSize);
                refillBoard(gridSize);
            } while (countMatches(gridSize) > 0);
        }

        // 5. Check if the final clean board has moves
        moveFound = CheckForPossibleMoves(gridSize);
    }

    // Clean up state
    selectedR = -1; selectedC = -1;
    logicState = LOGIC_IDLE;
    std::cout << "Board Shuffled to unlock moves.\n";
}

//END NEW BOARD MANAGEMENT FUNCTIONS


//MAIN SFML FUNCTION

int main() {
    std::srand(static_cast<unsigned int>(std::time(nullptr)));

    sf::RenderWindow window(sf::VideoMode(1800, 1100), "Candy Crush Clone");
    window.setFramerateLimit(60);

    //... (Rest of main function is unchanged and is included for completeness)

        //ASSET LOADING
    if (!mainFont.loadFromFile("assets/funsized.ttf")) std::cerr << "Failed to load funsized font\n";

    sf::Font volumeFont;
    if (!volumeFont.loadFromFile("assets/SuperAdorable-MAvyp.ttf")) std::cerr << "Failed to load volume font\n";

    sf::Font levelInsideFont;
    if (!levelInsideFont.loadFromFile("assets/DroplineRegular-Wpegz.otf")) std::cerr << "Failed to load level detail font\n";

    if (!candyTextures[0].loadFromFile("assets/candy1.png")) std::cout << "Error loading candy1\n";
    if (!candyTextures[1].loadFromFile("assets/candy2.png")) std::cout << "Error loading candy2\n";
    if (!candyTextures[2].loadFromFile("assets/candy3.png")) std::cout << "Error loading candy3\n";
    if (!candyTextures[3].loadFromFile("assets/candy4.png")) std::cout << "Error loading candy4\n";
    if (!candyTextures[4].loadFromFile("assets/candy5.png")) std::cout << "Error loading candy5\n";
    if (!candyTextures[5].loadFromFile("assets/chocolate.png")) std::cout << "Error loading chocolate booster\n";

    sf::Texture starTexture;
    if (!starTexture.loadFromFile("assets/star.png")) std::cout << "Error loading star.png\n";

    // Sound Loading
    if (!successSoundBuffer1.loadFromFile("assets/success sound 1.mp3")) std::cerr << "Failed to load success sound 1\n";
    if (!successSoundBuffer2.loadFromFile("assets/success sound 2.mp3")) std::cerr << "Failed to load success sound 2\n";
    if (!failSoundBuffer1.loadFromFile("assets/fail sound 1.mp3")) std::cerr << "Failed to load fail sound 1\n";
    if (!failSoundBuffer2.loadFromFile("assets/fail sound 2.mp3")) std::cerr << "Failed to load fail sound 2\n";

    if (!boosterSoundBuffer1.loadFromFile("assets/booster sound.ogg")) std::cerr << "Failed to load booster sound 1\n";
    if (!boosterSoundBuffer2.loadFromFile("assets/booster sound 2.ogg")) std::cerr << "Failed to load booster sound 2\n";
    if (!boosterSoundBuffer3.loadFromFile("assets/booster sound 3.mp3")) std::cerr << "Failed to load booster sound 3\n";


    successSound1.setBuffer(successSoundBuffer1);
    successSound2.setBuffer(successSoundBuffer2);
    failSound1.setBuffer(failSoundBuffer1);
    failSound2.setBuffer(failSoundBuffer2);

    boosterSound1.setBuffer(boosterSoundBuffer1);
    boosterSound2.setBuffer(boosterSoundBuffer2);
    boosterSound3.setBuffer(boosterSoundBuffer3);

    // Music Initialization
    if (!backgroundMusic.openFromFile("assets/Candy Crush Menu.ogg")) std::cerr << "Failed to load menu music!\n";
    if (!levelStateMusic.openFromFile("assets/level state sound.ogg")) std::cerr << "Failed to load level state sound!\n";

    backgroundMusic.setLoop(true);
    levelStateMusic.setLoop(true);
    backgroundMusic.play();
    backgroundMusic.setVolume(volumeValue * 100);


    //LOAD GAME PROGRESS
    loadGameProgress();

    //UI OBJECTS (Background, Menu, Settings, Levels)
    sf::Texture backgroundTexture, loadingTexture, menuBgTexture;
    backgroundTexture.loadFromFile("assets/background.jpg");
    loadingTexture.loadFromFile("assets/LoadingPage.jpg");
    menuBgTexture.loadFromFile("assets/mainBg.png");

    sf::Sprite background(backgroundTexture);
    background.setScale(1800.f / backgroundTexture.getSize().x, 1100.f / backgroundTexture.getSize().y);

    float sideWidth = (1800.f - 800.f) / 2.f;
    sf::RectangleShape leftBlur(sf::Vector2f(sideWidth, 1100));
    leftBlur.setPosition(0, 0);
    leftBlur.setFillColor(sf::Color(0, 0, 0, 150));
    sf::RectangleShape rightBlur(sf::Vector2f(sideWidth, 1100));
    rightBlur.setPosition(800 + sideWidth, 0);
    rightBlur.setFillColor(sf::Color(0, 0, 0, 150));

    sf::RectangleShape start_main(sf::Vector2f(800, 1100));
    start_main.setPosition(500, 0);
    start_main.setFillColor(sf::Color(255, 165, 0)); // Orange

    sf::Text main_text("crush\nforce\n  3.0", mainFont, 100);
    main_text.setFillColor(sf::Color(255, 255, 153));
    main_text.setPosition(700, 400);

    sf::Sprite loadingPage(loadingTexture);
    loadingPage.setScale(800.f / loadingTexture.getSize().x, 1100.f / loadingTexture.getSize().y);
    loadingPage.setPosition(500, 0);

    // Menu Objects
    sf::Sprite mainBg(menuBgTexture);
    mainBg.setScale(800.f / menuBgTexture.getSize().x, 1100.f / menuBgTexture.getSize().y);
    mainBg.setPosition(500, 0);

    sf::Texture PlayBtnTexture, instructionManualBtnTexture, SettingBtnTexture;
    PlayBtnTexture.loadFromFile("assets/PlayButton.png");

    if (!instructionManualBtnTexture.loadFromFile("assets/instructionManual.png")) std::cerr << "Failed to load instructionManual.jpg\n";
    sf::Texture instructionManualPageTexture;
    if (!instructionManualPageTexture.loadFromFile("assets/instructionManualPage.png")) std::cerr << "Failed to load instructionManualPage.png\n";

    SettingBtnTexture.loadFromFile("assets/SettingsButton.png");

    sf::Sprite PlayBtn(PlayBtnTexture);
    PlayBtn.setScale(300.f / PlayBtnTexture.getSize().x, 100.f / PlayBtnTexture.getSize().y);
    PlayBtn.setPosition(750, 690);

    sf::Sprite instructionManualBtn(instructionManualBtnTexture);
    instructionManualBtn.setScale(340.f / instructionManualBtnTexture.getSize().x, 110.f / instructionManualBtnTexture.getSize().y);
    instructionManualBtn.setPosition(730, 800);

    sf::Sprite SettingBtn(SettingBtnTexture);
    SettingBtn.setScale(70.f / SettingBtnTexture.getSize().x, 70.f / SettingBtnTexture.getSize().y);
    SettingBtn.setPosition(540, 900);

    sf::Sprite PlayBtnShadow = PlayBtn; PlayBtnShadow.setColor(sf::Color(0, 0, 0, 150)); PlayBtnShadow.setPosition(PlayBtn.getPosition().x + 5, PlayBtn.getPosition().y + 5);

    sf::Sprite instructionManualBtnShadow = instructionManualBtn; instructionManualBtnShadow.setColor(sf::Color(0, 0, 0, 150)); instructionManualBtnShadow.setPosition(instructionManualBtn.getPosition().x + 5, instructionManualBtn.getPosition().y + 5);

    sf::Sprite SettingBtnShadow = SettingBtn; SettingBtnShadow.setColor(sf::Color(0, 0, 0, 150)); SettingBtnShadow.setPosition(SettingBtn.getPosition().x + 5, SettingBtn.getPosition().y + 5);

    sf::Sprite instructionManualPage(instructionManualPageTexture);
    instructionManualPage.setScale(800.f / instructionManualPageTexture.getSize().x, 1100.f / instructionManualPageTexture.getSize().y);
    instructionManualPage.setPosition(500, 0);


    // Settings Objects
    sf::Texture settingBgUpTexture, settingBgTexture, generalTexture, crossTexture;
    settingBgUpTexture.loadFromFile("assets/clouds.jpg");
    settingBgTexture.loadFromFile("assets/settingBg.jpg");
    generalTexture.loadFromFile("assets/general.png");
    crossTexture.loadFromFile("assets/cross.png");

    sf::Sprite settingBgUp(settingBgUpTexture); settingBgUp.setScale(800.f / settingBgUpTexture.getSize().x, 200.f / settingBgUpTexture.getSize().y); settingBgUp.setPosition(500, 0);
    sf::Sprite settingBg(settingBgTexture); settingBg.setScale(800.f / settingBgTexture.getSize().x, 900.f / settingBgTexture.getSize().y); settingBg.setPosition(500, 200);
    sf::Sprite general(generalTexture); general.setScale(800.f / generalTexture.getSize().x, 100.f / generalTexture.getSize().y); general.setPosition(500, 110);
    sf::Sprite cross(crossTexture); cross.setScale(80.f / crossTexture.getSize().x, 80.f / crossTexture.getSize().y); cross.setPosition(1150, 30);
    sf::Sprite crossShadow = cross; crossShadow.setColor(sf::Color(0, 0, 0, 120)); crossShadow.setPosition(cross.getPosition().x + 4, cross.getPosition().y + 4);

    sf::RectangleShape volumePanel(sf::Vector2f(700.f, 300.f)); volumePanel.setPosition(550, 300); volumePanel.setFillColor(sf::Color(255, 240, 250)); volumePanel.setOutlineThickness(4); volumePanel.setOutlineColor(sf::Color(255, 200, 220));
    sf::Text volumeLabel("Volume:", volumeFont, 70); volumeLabel.setFillColor(sf::Color(150, 30, 60)); volumeLabel.setPosition(580, 320);
    sf::RectangleShape volumeBar(sf::Vector2f(450.f, 14.f)); volumeBar.setFillColor(sf::Color(255, 180, 210)); volumeBar.setPosition(600, 480);
    sf::CircleShape volumeKnob(25.f); volumeKnob.setFillColor(sf::Color(20, 110, 255)); volumeKnob.setOutlineThickness(4); volumeKnob.setOutlineColor(sf::Color::White);
    float knobRadius = 25.f; volumeKnob.setRadius(knobRadius); volumeKnob.setOrigin(knobRadius, knobRadius);
    sf::RectangleShape toggleBase(sf::Vector2f(80.f, 40.f)); toggleBase.setPosition(1100, 350);
    sf::CircleShape toggleCircle(18.f); toggleCircle.setFillColor(sf::Color::White);

    if (volumeEnabled) {
        volumeKnob.setPosition(volumeBar.getPosition().x + volumeValue * volumeBar.getSize().x, volumeBar.getPosition().y + volumeBar.getSize().y / 2);
    }
    else {
        volumeKnob.setPosition(volumeBar.getPosition().x, volumeBar.getPosition().y + volumeBar.getSize().y / 2);
    }

    toggleBase.setFillColor(volumeEnabled ? sf::Color(100, 230, 120) : sf::Color(220, 120, 140));
    toggleCircle.setPosition(toggleBase.getPosition().x + (volumeEnabled ? 40 : 2), toggleBase.getPosition().y + 2);


    // Levels Objects
    sf::Texture levelsBgTexture, livesBgTexture, livescandyBgTexture;
    levelsBgTexture.loadFromFile("assets/levelsBg.jpg");
    livesBgTexture.loadFromFile("assets/livesBg.png");
    livescandyBgTexture.loadFromFile("assets/candy.png");

    sf::Sprite levelsBg(levelsBgTexture); levelsBg.setScale(800.f / levelsBgTexture.getSize().x, 1100.f / levelsBgTexture.getSize().y); levelsBg.setPosition(500, 0);
    sf::Sprite livesBg(livesBgTexture); livesBg.setScale(350.f / livesBgTexture.getSize().x, 150.f / livesBgTexture.getSize().y); livesBg.setPosition(510, 20);
    sf::Sprite livescandyBg(livescandyBgTexture); livescandyBg.setScale(60.f / livescandyBgTexture.getSize().x, 60.f / livescandyBgTexture.getSize().y); livescandyBg.setPosition(610, 45);

    float levelPositionsX[10] = { 850, 975, 1050, 1000, 875, 739, 785, 900, 979, 1020 };
    float levelPositionsY[10] = { 1010, 930, 810, 670, 590, 520, 395, 315, 200, 60 };

    sf::CircleShape levelCircles[10];
    sf::Text levelNumbers[10];
    sf::CircleShape levelStarsDisplay[10][3];

    for (int i = 0; i < 10; i++) {
        levelCircles[i].setRadius(45.f);
        levelCircles[i].setOrigin(50.f, 50.f);

        levelCircles[i].setPosition(levelPositionsX[i], levelPositionsY[i]);

        levelNumbers[i].setFont(mainFont);
        levelNumbers[i].setCharacterSize(30);
        levelNumbers[i].setFillColor(sf::Color::Black);
        levelNumbers[i].setString(std::to_string(i + 1));
        levelNumbers[i].setOrigin(levelNumbers[i].getLocalBounds().width / 2, levelNumbers[i].getLocalBounds().height / 2);

        levelNumbers[i].setPosition(levelPositionsX[i] - 5, levelPositionsY[i] - 10);

        for (int j = 0; j < 3; j++) {
            levelStarsDisplay[i][j].setRadius(10.f);
            levelStarsDisplay[i][j].setOrigin(10.f, 10.f);
            \
                levelStarsDisplay[i][j].setPosition(levelPositionsX[i] - 30 + j * 25, levelPositionsY[i] + 60);
        }
    }


    // Level 1 Game UI Objects
    sf::Texture gridUpTexture, gridTexture, iconTexture;
    gridUpTexture.loadFromFile("assets/gridUp.jpg");
    gridTexture.loadFromFile("assets/Candygrid.jpg");
    iconTexture.loadFromFile("assets/icon.png");

    sf::Sprite gridUp(gridUpTexture); gridUp.setScale(800.f / gridUpTexture.getSize().x, 300.f / gridUpTexture.getSize().y); gridUp.setPosition(500, 0);
    sf::Sprite grid(gridTexture); grid.setScale(800.f / gridTexture.getSize().x, 900.f / gridTexture.getSize().y); grid.setPosition(500, 200);
    sf::Sprite icon(iconTexture); icon.setScale(200.f / iconTexture.getSize().x, 200.f / iconTexture.getSize().y); icon.setPosition(845, 0);

    sf::Sprite quitBtn(crossTexture);
    quitBtn.setScale(80.f / crossTexture.getSize().x, 80.f / crossTexture.getSize().y);
    quitBtn.setPosition(1150, 30);
    sf::Sprite quitBtnShadow = quitBtn; quitBtnShadow.setColor(sf::Color(0, 0, 0, 120)); quitBtnShadow.setPosition(quitBtn.getPosition().x + 4, quitBtn.getPosition().y + 4);

    sf::Text endLevelText("", mainFont, 80);
    endLevelText.setOutlineColor(sf::Color(150, 30, 60));
    endLevelText.setOutlineThickness(5);
    endLevelText.setPosition(900, 550);
    endLevelText.setOrigin(0, 0);
    endLevelText.setCharacterSize(50);

    // Initial grid setup
    restartLevel(currentLevel);

    //MAIN GAME LOOP

    while (window.isOpen())
    {
        sf::Event event;
        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed) {
                saveGameProgress();
                window.close();
            }

            if (event.type == sf::Event::KeyPressed) {

                if (event.key.code == sf::Keyboard::P) {
                    if (gameState == STATE_LEVEL1) {
                        pauseGame();
                    }
                    else if (gameState == STATE_PAUSED) {
                        resumeGame();
                    }
                }
                if (gameState == STATE_MENU && event.key.code == sf::Keyboard::S)
                    gameState = STATE_SETTINGS;
                if (gameState == STATE_SETTINGS && event.key.code == sf::Keyboard::C)
                    gameState = STATE_MENU;

                if (gameState == STATE_MENU && event.key.code == sf::Keyboard::I) {
                    gameState = STATE_INSTRUCTIONS;
                }
                if (gameState == STATE_INSTRUCTIONS && event.key.code == sf::Keyboard::C) {
                    gameState = STATE_MENU;
                }

                if (gameState != STATE_LIVES_WAITING) {
                    if (gameState == STATE_MENU && event.key.code == sf::Keyboard::P) {
                        gameState = STATE_LEVELS;
                    }
                    if (gameState == STATE_LEVELS && event.key.code == sf::Keyboard::B) {
                        gameState = STATE_MENU;
                    }
                }
                if (event.key.code == sf::Keyboard::U && volumeEnabled)
                {
                    volumeValue += 0.05f;
                    if (volumeValue > 1.f)
                        volumeValue = 1.f;
                }
                if (event.key.code == sf::Keyboard::D && volumeEnabled) { volumeValue -= 0.05f; if (volumeValue < 0.f) volumeValue = 0.f; }
                if (event.key.code == sf::Keyboard::M) { volumeEnabled = !volumeEnabled; if (!volumeEnabled) { previousVolumeValue = volumeValue; volumeValue = 0.f; } else { volumeValue = previousVolumeValue; } }
                else if ((gameState == STATE_LEVEL1 || gameState == STATE_LEVEL_END_PREVIEW || gameState == STATE_PAUSED) && event.key.code == sf::Keyboard::Q) {
                    {
                        if (gameState == STATE_LEVEL_END_PREVIEW)
                        {
                            gameState = nextLevelState;
                        }
                        else
                        {
                            quitLevel();
                        }
                    }


                    if (gameState == STATE_PAUSED) {
                        sf::RectangleShape overlay(sf::Vector2f(1800, 1100));
                        overlay.setFillColor(sf::Color(0, 0, 0, 150)); // Dark overlay
                        window.draw(overlay);

                        sf::Text pauseText("PAUSED\n\nPress 'P' to Resume", mainFont, 80);
                        pauseText.setFillColor(sf::Color::Yellow);
                        pauseText.setOutlineColor(sf::Color::Red);
                        pauseText.setOutlineThickness(3.f);
                        pauseText.setOrigin(pauseText.getLocalBounds().width / 2.f, pauseText.getLocalBounds().height / 2.f);
                        pauseText.setPosition(900, 550);
                        window.draw(pauseText);
                    }
                }
                if (gameState == STATE_LEVELS) {
                    int lvl = -1;
                    if (event.key.code >= sf::Keyboard::Num1 && event.key.code <= sf::Keyboard::Num9) {
                        lvl = event.key.code - sf::Keyboard::Num1 + 1;
                    }
                    else if (event.key.code == sf::Keyboard::Num0) {
                        lvl = 10;
                    }
                    if (lvl != -1 && lvl <= maxUnlockedLevel && lives > 0) {
                        currentLevel = lvl;
                        gameState = STATE_LEVEL1;
                        restartLevel(currentLevel);

                        backgroundMusic.stop();
                        levelStateMusic.setVolume(volumeValue * 100.f);
                        levelStateMusic.play();
                    }
                }

                if (gameState == STATE_MENU && event.key.code == sf::Keyboard::B) {
                    levelStateMusic.stop();
                    backgroundMusic.setVolume(volumeValue * 100.f);
                    if (backgroundMusic.getStatus() != sf::SoundSource::Playing && volumeEnabled) {
                        backgroundMusic.play();
                    }
                }
            }

            if (event.type == sf::Event::MouseButtonPressed) {
                sf::Vector2f mousePos = window.mapPixelToCoords({ event.mouseButton.x, event.mouseButton.y });

                if (gameState == STATE_MENU) {
                    if (instructionManualBtn.getGlobalBounds().contains(mousePos)) {
                        gameState = STATE_INSTRUCTIONS;
                    }
                    if (PlayBtn.getGlobalBounds().contains(mousePos)) {
                        gameState = STATE_LEVELS;
                    }
                    if (SettingBtn.getGlobalBounds().contains(mousePos)) {
                        gameState = STATE_SETTINGS;
                    }
                }

                if (gameState == STATE_LEVELS) {
                    for (int i = 0; i < 10; ++i) {
                        // Use levelPositionsX[i] and levelPositionsY[i] to manually check bounds
                        // The CircleShape itself already has the correct position set from the initialization
                        if (levelCircles[i].getGlobalBounds().contains(mousePos)) {
                            int lvl = i + 1;
                            if (lvl <= maxUnlockedLevel && lives > 0) {
                                currentLevel = lvl;
                                gameState = STATE_LEVEL1;
                                restartLevel(currentLevel);

                                backgroundMusic.stop();
                                levelStateMusic.setVolume(volumeValue * 100.f);
                                levelStateMusic.play();

                                break;
                            }
                        }
                    }
                }

                if ((gameState == STATE_LEVEL1 || gameState == STATE_LEVEL_END_PREVIEW) && quitBtn.getGlobalBounds().contains(mousePos)) {
                    if (gameState == STATE_LEVEL_END_PREVIEW) {
                        gameState = nextLevelState;
                    }
                    else {
                        quitLevel();
                    }
                }

                if (gameState == STATE_LEVEL1 && logicState == LOGIC_IDLE) {
                    int gridSize = getGridSize(currentLevel);
                    float cellSize = 75.f; float gap = 10.f;

                    const float GAME_AREA_X = 500.f;
                    const float GAME_AREA_Y = 200.f;
                    const float GAME_AREA_WIDTH = 800.f;
                    const float GAME_AREA_HEIGHT = 900.f;

                    float totalContentWidth = gridSize * cellSize + (gridSize - 1) * gap;

                    float startX = GAME_AREA_X + (GAME_AREA_WIDTH - totalContentWidth) / 2.f;
                    float startY = GAME_AREA_Y + (GAME_AREA_HEIGHT - totalContentWidth) / 2.f;

                    int clickedR = (int)((mousePos.y - startY) / (cellSize + gap));
                    int clickedC = (int)((mousePos.x - startX) / (cellSize + gap));

                    if (clickedR >= 0 && clickedR < gridSize && clickedC >= 0 && clickedC < gridSize) {
                        if (selectedR == -1) {
                            selectedR = clickedR; selectedC = clickedC;
                        }
                        else if (areAdjacent(selectedR, selectedC, clickedR, clickedC)) {
                            secondR = clickedR; secondC = clickedC;
                            swapRequested = true;
                            logicState = LOGIC_SWAPPING;
                        }
                        else {
                            selectedR = clickedR; selectedC = clickedC;
                        }
                    }
                    else {
                        selectedR = selectedC = -1;
                    }
                }
            }
        } // end pollEvent

        //GLOBAL UPDATES
        backgroundMusic.setVolume(volumeValue * 100.f);
        levelStateMusic.setVolume(volumeValue * 100.f);

        successSound1.setVolume(volumeValue * 100.f);
        successSound2.setVolume(volumeValue * 100.f);
        failSound1.setVolume(volumeValue * 100.f);
        failSound2.setVolume(volumeValue * 100.f);
        boosterSound1.setVolume(volumeValue * 100.f);
        boosterSound2.setVolume(volumeValue * 100.f);
        boosterSound3.setVolume(volumeValue * 100.f);

        volumeKnob.setPosition(volumeBar.getPosition().x + volumeValue * volumeBar.getSize().x, volumeBar.getPosition().y + volumeBar.getSize().y / 2);
        toggleBase.setFillColor(volumeEnabled ? sf::Color(100, 230, 120) : sf::Color(220, 120, 140));
        toggleCircle.setPosition(toggleBase.getPosition().x + (volumeEnabled ? 40 : 2), toggleBase.getPosition().y + 2);

        //SOUND SEQUENCING LOGIC
        if (gameState == STATE_FAIL && !failSound2Played) {
            if (failSound1.getStatus() == sf::Sound::Stopped) {
                failSound2.play();
                failSound2Played = true;
            }
        }

        //STATE TRANSITIONS and TIMERS
        float t = transitionClock.getElapsedTime().asSeconds();
        if (gameState == STATE_INTRO && t > INTRO_DISPLAY_TIME + FADE_OUT_TIME + LOADING_SHOW_TIME) {

            if (lives <= 0 && livesEndTimestamp != 0) {
                gameState = STATE_LIVES_WAITING;
            }
            else {
                gameState = STATE_MENU;
            }
        }

        if (gameState != STATE_LIVES_WAITING) {
            if ((gameState == STATE_WIN || gameState == STATE_ACHIEVEMENT || gameState == STATE_FAIL) && endLevelClock.getElapsedTime().asSeconds() > END_LEVEL_SCREEN_DURATION) {
                GameState previousState = gameState;
                gameState = STATE_LEVELS;

                if (previousState != STATE_LEVELS) {
                    levelStateMusic.stop();
                    if (backgroundMusic.getStatus() != sf::SoundSource::Playing && volumeEnabled) {
                        backgroundMusic.setVolume(volumeValue * 100.f);
                        backgroundMusic.play();
                    }
                }
            }
        }

        if (gameState == STATE_LEVEL_END_PREVIEW && levelEndPreviewClock.getElapsedTime().asSeconds() >= LEVEL_END_PREVIEW_DURATION) {
            gameState = nextLevelState;
        }

        //DRAWING LOGIC
        window.clear();
        window.draw(background);
        window.draw(leftBlur);
        window.draw(rightBlur);

        if (gameState == STATE_INTRO)
        {
            float orangeEnd = INTRO_DISPLAY_TIME + FADE_OUT_TIME;
            if (t < orangeEnd) {
                float alpha = 255.f;
                if (t > INTRO_DISPLAY_TIME) alpha = 255.f * (1.f - (t - INTRO_DISPLAY_TIME) / FADE_OUT_TIME);

                start_main.setFillColor(sf::Color(255, 165, 0, (sf::Uint8)alpha));
                main_text.setFillColor(sf::Color(255, 255, 153, (sf::Uint8)alpha));
                window.draw(start_main);
                window.draw(main_text);

                if (t > INTRO_DISPLAY_TIME) {
                    float invAlpha = 255 - (sf::Uint8)alpha;
                    loadingPage.setColor(sf::Color(255, 255, 255, (sf::Uint8)invAlpha));
                    window.draw(loadingPage);
                }
            }
            else {
                loadingPage.setColor(sf::Color(255, 255, 255, 255));
                window.draw(loadingPage);
            }
        }
        else if (gameState == STATE_MENU)
        {
            window.draw(mainBg);
            window.draw(PlayBtnShadow);
            window.draw(instructionManualBtnShadow);
            window.draw(SettingBtnShadow);
            window.draw(instructionManualBtn);
            window.draw(PlayBtn);
            window.draw(SettingBtn);
        }
        else if (gameState == STATE_INSTRUCTIONS)
        {
            window.draw(instructionManualPage);
        }
        else if (gameState == STATE_SETTINGS)
        {
            window.draw(settingBg);
            window.draw(settingBgUp);
            window.draw(general);
            window.draw(volumePanel);
            window.draw(volumeLabel);
            window.draw(volumeBar);
            window.draw(volumeKnob);
            window.draw(toggleBase);
            window.draw(toggleCircle);
            window.draw(crossShadow);
            window.draw(cross);
            window.draw(leftBlur);
            window.draw(rightBlur);
        }
        else if (gameState == STATE_LEVELS)
        {
            if (levelStateMusic.getStatus() == sf::SoundSource::Playing) {
                levelStateMusic.stop();
            }
            if (backgroundMusic.getStatus() != sf::SoundSource::Playing && volumeEnabled) {
                backgroundMusic.setVolume(volumeValue * 100.f);
                backgroundMusic.play();
            }

            window.draw(levelsBg);
            window.draw(livesBg);
            window.draw(livescandyBg);

            sf::Text livesText("Lives : " + std::to_string(lives), levelInsideFont, 30);
            livesText.setFillColor(sf::Color::Red);
            livesText.setPosition(680, 60);
            window.draw(livesText);

            sf::Text totalStarsText("Total Stars: " + std::to_string(totalStars), levelInsideFont, 30);
            totalStarsText.setFillColor(sf::Color(204, 153, 0));
            totalStarsText.setPosition(630, 100);
            window.draw(totalStarsText);

            // Display High Scores (NEW)
            for (int i = 0; i < 10; ++i) {
                if (highScores[i] > 0) {
                    sf::Text hsText("HS: " + std::to_string(highScores[i]), levelInsideFont, 20);
                    hsText.setFillColor(sf::Color::Blue);
                    hsText.setPosition(levelPositionsX[i] - 30, levelPositionsY[i] + 80);
                    window.draw(hsText);
                }
            }
            // End Display High Scores


            for (int i = 0; i < 10; i++) {
                int lvl = i + 1;
                bool isUnlocked = (lvl <= maxUnlockedLevel);

                if (lives > 0) {
                    levelCircles[i].setFillColor(isUnlocked ? sf::Color(255, 200, 0) : sf::Color(100, 100, 100));
                }
                else {
                    levelCircles[i].setFillColor(sf::Color(150, 50, 50));
                }

                if (!isUnlocked) {
                    levelCircles[i].setFillColor(sf::Color(150, 50, 50));
                    levelCircles[i].setOutlineThickness(0);
                }
                else {
                    levelCircles[i].setOutlineThickness(0);
                }

                window.draw(levelCircles[i]);
                window.draw(levelNumbers[i]);

                for (int j = 0; j < 3; j++) {
                    if (j < levelStars[i]) levelStarsDisplay[i][j].setFillColor(sf::Color::Yellow);
                    else levelStarsDisplay[i][j].setFillColor(sf::Color(100, 100, 100));
                    window.draw(levelStarsDisplay[i][j]);
                }
            }

            if (lives <= 0) {
                sf::Text waitMsg("All levels locked. Wait for lives or buy them (not implemented)", levelInsideFont, 20);
                waitMsg.setFillColor(sf::Color::Red);
                waitMsg.setPosition(750, 200);
                window.draw(waitMsg);
            }
        }
        else if (gameState == STATE_LEVEL1 || gameState == STATE_LEVEL_END_PREVIEW)
        {
            window.draw(gridUp);
            window.draw(grid);
            window.draw(icon);

            window.draw(quitBtnShadow);
            window.draw(quitBtn);

            window.draw(livesBg);

            sf::Text starsLabel("Stars:", levelInsideFont, 30);
            starsLabel.setFillColor(sf::Color(204, 153, 0));
            starsLabel.setPosition(620, 45);
            window.draw(starsLabel);

            sf::Sprite starSprite(starTexture);
            float starSize = 35.f;
            float startY = 45.f;
            float startX = 700.f;
            float scale = starSize / starTexture.getSize().x;
            starSprite.setScale(scale, scale);

            for (int i = 0; i < currentlivestars; ++i) {
                starSprite.setPosition(startX + i * (starSize + 5.f), startY);
                window.draw(starSprite);
            }

            sf::Text requirementText("Moves : " + std::to_string(currentMoves), levelInsideFont, 30);
            requirementText.setFillColor(sf::Color(0, 100, 0)); requirementText.setPosition(620, 75); window.draw(requirementText);

            sf::Text scoreDisplay("Score : " + std::to_string(score), levelInsideFont, 30);
            scoreDisplay.setFillColor(sf::Color::Red); scoreDisplay.setPosition(620, 105); window.draw(scoreDisplay);

            int gridSize = getGridSize(currentLevel);
            float cellSize = 75.f; float gap = 10.f;

            //GAME LOGIC STATE MACHINE UPDATE (Only runs in STATE_LEVEL1)
            if (gameState == STATE_LEVEL1) {
                int matchCount = 0;
                switch (logicState) {
                case LOGIC_SWAPPING:
                    SwapCandies(selectedR, selectedC, secondR, secondC);
                    if (logicState == LOGIC_SWAPPING) {
                        logicState = LOGIC_CHECKING;
                    }
                    break;
                case LOGIC_CHECKING:
                    matchCount = countMatches(gridSize);
                    if (matchCount > 0) {
                        if (swapRequested) {
                            currentMoves--;
                        }
                        playLevel(matchCount * 100);

                        if (gameState != STATE_LEVEL1) break;

                        bool boosterCreatedNow = (boosterR != -1 && boosterC != -1);

                        removeMatches(gridSize);

                        logicState = LOGIC_FALLING;
                        swapRequested = false;
                    }
                    else if (swapRequested) {
                        // Swap back if no match was found (only swaps if not a booster activation)
                        SwapCandies(selectedR, selectedC, secondR, secondC);
                        swapRequested = false;
                        selectedR = -1; secondR = -1;
                        logicState = LOGIC_IDLE;
                    }
                    else {
                        logicState = LOGIC_IDLE;
                    }
                    break;
                case LOGIC_FALLING:
                    applyGravity(gridSize);
                    logicState = LOGIC_REFILLING;
                    break;
                case LOGIC_REFILLING:
                    refillBoard(gridSize);
                    logicState = LOGIC_CHECKING;
                    break;
                case LOGIC_BOOSTER_EXPLODING:
                    if (boosterExplosionClock.getElapsedTime().asSeconds() >= BOOSTER_EXPLOSION_DURATION) {
                        clearWrappedEffect(boosterExplodeR, boosterExplodeC, gridSize);
                        boosterExplodeR = -1;
                        boosterExplodeC = -1;

                        playLevel(0);

                        logicState = LOGIC_FALLING;
                    }
                    break;
                case LOGIC_IDLE:
                default:
                    if (currentMoves <= 0) {
                        playLevel(0);
                    }
                    swapRequested = false;

                    if (!CheckForPossibleMoves(gridSize)) {
                        ShuffleBoard(gridSize);
                    }

                    break;
                }
            }

            //DRAW CANDIES
            for (int i = 0; i < gridSize; i++) {
                for (int j = 0; j < gridSize; j++) {
                    CandyType type = level1Candies[i][j]; //Get enum

                    if (type != TYPE_EMPTY) {
                        // Use the enum value as the texture index
                        sf::Sprite candySprite(candyTextures[type]);

                        float cellSizeFinal = 75.f;
                        float scaleX = cellSizeFinal / candyTextures[type].getSize().x;
                        float scaleY = cellSizeFinal / candyTextures[type].getSize().y;
                        candySprite.setScale(scaleX, scaleY);

                        sf::Vector2f pos = getCandyPosition(i, j, gridSize, cellSizeFinal, gap);
                        candySprite.setPosition(pos);

                        if ((i == selectedR && j == selectedC)) {
                            candySprite.setColor(sf::Color(255, 255, 255, 180));
                        }
                        else {
                            candySprite.setColor(sf::Color::White);
                        }

                        window.draw(candySprite);
                    }
                }
            }

            //BOOSTER EXPLOSION ANIMATION
            if (gameState == STATE_LEVEL1 && logicState == LOGIC_BOOSTER_EXPLODING && boosterExplodeR != -1) {
                float progress = boosterExplosionClock.getElapsedTime().asSeconds() / BOOSTER_EXPLOSION_DURATION;
                float cellSizeFinal = 75.f; float gap = 10.f;

                float alphaFloat = std::sin(progress * 3.14159f * 2.f) * 255.f;
                sf::Uint8 alpha = static_cast<sf::Uint8>(maximum(0.0f, alphaFloat));

                sf::Color pulseColor(255, 255, 255, alpha);

                sf::RectangleShape rowHighlight(sf::Vector2f(gridSize * cellSizeFinal + (gridSize + 1) * gap, cellSizeFinal + 2 * gap));
                rowHighlight.setFillColor(pulseColor);
                sf::Vector2f rowPos = getCandyPosition(boosterExplodeR, 0, gridSize, cellSizeFinal, gap);
                rowHighlight.setPosition(rowPos.x - gap, rowPos.y - gap);
                window.draw(rowHighlight);

                sf::RectangleShape colHighlight(sf::Vector2f(cellSizeFinal + 2 * gap, gridSize * cellSizeFinal + (gridSize + 1) * gap));
                colHighlight.setFillColor(pulseColor);
                sf::Vector2f colPos = getCandyPosition(0, boosterExplodeC, gridSize, cellSizeFinal, gap);
                colHighlight.setPosition(colPos.x - gap, colPos.y - gap);
                window.draw(colHighlight);
            }


            //LEVEL END PREVIEW LOGIC
            if (gameState == STATE_LEVEL_END_PREVIEW) {
                sf::RectangleShape overlay(sf::Vector2f(1800, 1100));
                overlay.setFillColor(sf::Color(0, 0, 0, 100));
                window.draw(overlay);

                sf::Text pauseText("Level Passed!", levelInsideFont, 60);
                pauseText.setFillColor(sf::Color(255, 255, 0));
                pauseText.setOutlineColor(sf::Color(0, 0, 0));
                pauseText.setOutlineThickness(3);
                pauseText.setOrigin(pauseText.getLocalBounds().width / 2.f, pauseText.getLocalBounds().height / 2.f);
                pauseText.setPosition(900, 550);
                window.draw(pauseText);
            }
        }
        else if (gameState == STATE_WIN)
        {
            sf::RectangleShape overlay(sf::Vector2f(1800, 1100));
            overlay.setFillColor(sf::Color(0, 0, 0, 150));
            window.draw(overlay);

            endLevelText.setString("LEVEL " + std::to_string(currentLevel) + " PASSED!\n\nAchievement unlocked! \nFAST student who actually \npassed something ");
            endLevelText.setCharacterSize(40);
            endLevelText.setFillColor(sf::Color(0, 200, 0));
            endLevelText.setOutlineColor(sf::Color(0, 80, 0));

            endLevelText.setOrigin(endLevelText.getLocalBounds().width / 2.f, endLevelText.getLocalBounds().height / 2.f);
            endLevelText.setPosition(900, 550);
            window.draw(endLevelText);
        }
        else if (gameState == STATE_ACHIEVEMENT)
        {
            sf::RectangleShape overlay(sf::Vector2f(1800, 1100));
            overlay.setFillColor(sf::Color(0, 0, 0, 150));
            window.draw(overlay);

            endLevelText.setString("LEVEL " + std::to_string(currentLevel) + " PASSED!\n\nAchievement unlocked! \nFAST student who actually \npassed something ");
            endLevelText.setCharacterSize(40);
            endLevelText.setFillColor(sf::Color(0, 200, 0));
            endLevelText.setOutlineColor(sf::Color(0, 80, 0));

            endLevelText.setOrigin(endLevelText.getLocalBounds().width / 2.f, endLevelText.getLocalBounds().height / 2.f);
            endLevelText.setPosition(900, 550);
            window.draw(endLevelText);
        }
        else if (gameState == STATE_FAIL)
        {
            sf::RectangleShape overlay(sf::Vector2f(1800, 1100));
            overlay.setFillColor(sf::Color(0, 0, 0, 150));
            window.draw(overlay);

            endLevelText.setString("LEVEL " + std::to_string(currentLevel) + " FAILED!\n\nTry Again! Unlike FAST labs\n this game actually gives \nsecond chances");
            endLevelText.setCharacterSize(40);
            endLevelText.setOutlineColor(sf::Color(100, 0, 0));
            endLevelText.setFillColor(sf::Color(255, 50, 50));

            endLevelText.setOrigin(endLevelText.getLocalBounds().width / 2.f, endLevelText.getLocalBounds().height / 2.f);
            endLevelText.setPosition(900, 550);
            window.draw(endLevelText);
        }
        else if (gameState == STATE_LIVES_WAITING)
        {
            if (levelStateMusic.getStatus() == sf::SoundSource::Playing) {
                levelStateMusic.stop();
            }
            if (backgroundMusic.getStatus() != sf::SoundSource::Playing && volumeEnabled) {
                backgroundMusic.setVolume(volumeValue * 100.f);
                backgroundMusic.play();
            }

            long long currentTime = (long long)time(NULL);
            float elapsed = (float)(currentTime - livesEndTimestamp);
            float remaining = LIVES_REFILL_TIME - elapsed;

            if (remaining <= 0.0f) {
                lives = 3;
                livesEndTimestamp = 0;
                saveGameProgress();
                gameState = STATE_LEVELS;

            }
            else {
                sf::RectangleShape overlay(sf::Vector2f(1800, 1100));
                overlay.setFillColor(sf::Color(0, 0, 0, 200));
                window.draw(overlay);

                string countdownText = "Your hearts are healing !\nPrepare for the comeback\n\nLives restore in " + std::to_string((int)std::ceil(remaining)) + " seconds\n\nReturning to level selection\n upon refill";
                endLevelText.setString(countdownText);
                endLevelText.setCharacterSize(40);
                endLevelText.setOutlineColor(sf::Color(255, 100, 0));
                endLevelText.setFillColor(sf::Color::Yellow);

                endLevelText.setOrigin(endLevelText.getLocalBounds().width / 2.f, endLevelText.getLocalBounds().height / 2.f);
                endLevelText.setPosition(900, 550);
                window.draw(endLevelText);
            }
        }
        endLevelText.setCharacterSize(50);
        endLevelText.setFillColor(sf::Color(255, 255, 255));
        endLevelText.setOutlineColor(sf::Color(150, 30, 60));

        window.display();

    }
    return 0;
}


