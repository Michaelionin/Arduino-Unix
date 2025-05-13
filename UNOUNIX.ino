#include <LiquidCrystal.h>

#define B1_PIN 7
#define B2_PIN 6
#define B3_PIN 5
#define B4_PIN 4

LiquidCrystal lcd(13, 12, 11, 10, 9, 8);

enum SystemState {
  MAIN_MENU,
  APP_TERMINAL,
  GAME_JUMPER,
  GAME_CLICKER,
  SCREENSAVER
};

SystemState currentState = MAIN_MENU;
int menuPos = 0;
const char* menuItems[] = {"Terminal", "Doggy Jumper", "Hacky Hack", "System Info"};

// Переменные для Doggy Jumper
bool isJumping = false;
int jumpPhase = 0;
int obstaclePos = 16;
int obstacleType = 0;
unsigned long lastUpdate = 0;
int score = 0;
bool gameActive = false;
unsigned long gameSpeed = 350;

// Переменные для Hacky Hack
unsigned long hackCount = 0;
int clickPower = 1;
unsigned long upgradeCost = 10;
unsigned long lastClickTime = 0;

String inputBuffer = "";
bool refreshDisplay = true;

unsigned long lastActivityTime = 0;
const unsigned long screensaverTimeout = 10000; // 10 секунд бездействия
int animPos = 0;
int animDirection = 1;
byte ship[8] = { // Собственный символ для анимации
  0b00000,
  0b01110,
  0b11111,
  0b11111,
  0b01110,
  0b00100,
  0b00000,
  0b00000
};

SystemState previousState = MAIN_MENU;

void setup() {
  Serial.begin(9600);
  Serial.println("Serial daemon started");
  Serial.println("Starting button driver...");
  pinMode(B1_PIN, INPUT_PULLUP);
  pinMode(B2_PIN, INPUT_PULLUP);
  pinMode(B3_PIN, INPUT_PULLUP);
  pinMode(B4_PIN, INPUT_PULLUP);
  Serial.println("OK");
  Serial.println("Starting lcd driver...");
  lcd.begin(16, 2);
  lcd.createChar(0, ship); // Регистрируем собственный символ
  Serial.println("OK");
  lastActivityTime = millis();
  Serial.println("Showing bootscreen...");
  lcd.setCursor(0, 0);
  lcd.print("Arduino UNIX 0.5");
  
  lcd.setCursor(1, 1);
  lcd.print("Initializing...");
  Serial.println("OK");
  delay(1000);
  Serial.println("Welcome to Arduino UNIX! Type help to help.");

}

void loop() {
  if(millis() - lastActivityTime > screensaverTimeout && currentState != SCREENSAVER) {
    previousState = currentState;
    currentState = SCREENSAVER;
    refreshDisplay = true;
  }

  handleButtons();
  
  if(Serial.available()) {
    char c = Serial.read();
    if(c == '\n') {
      processCommand(inputBuffer);
      inputBuffer = "";
    } else {
      inputBuffer += c;
    }
  }

  if(refreshDisplay) {
    switch(currentState) {
      case MAIN_MENU: drawMenu(); break;
      case APP_TERMINAL: drawTerminal(); break;
      case GAME_JUMPER: drawJumperGame(); break;
      case GAME_CLICKER: drawClickerGame(); break;
      case SCREENSAVER: drawScreensaver(); break;
    }
    refreshDisplay = false;
  }

  if(currentState == GAME_JUMPER && gameActive) {
    gameJumperLogic();
  }

  if(currentState == SCREENSAVER && millis() - lastUpdate > 300) {
    animPos += animDirection;
    if(animPos >= 13 || animPos <= 0) animDirection *= -1;
    refreshDisplay = true;
    lastUpdate = millis();
  }
}

void handleButtons() {
  if(digitalRead(B1_PIN) == LOW || digitalRead(B2_PIN) == LOW || 
     digitalRead(B3_PIN) == LOW || digitalRead(B4_PIN) == LOW) {
    lastActivityTime = millis();
    if(currentState == SCREENSAVER) {
      currentState = previousState;
      refreshDisplay = true;
      return;
    }
  }

  static unsigned long lastDebounce = 0;
  if(millis() - lastDebounce < 100) return;
  lastDebounce = millis();

  // Обработка кнопок для разных состояний
  switch(currentState) {
    case MAIN_MENU:
      handleMenuButtons();
      break;
      
    case GAME_JUMPER:
      handleJumperButtons();
      break;
      
    case GAME_CLICKER:
      handleClickerButtons();
      break;
      
    default:
      if(digitalRead(B3_PIN) == LOW) executeMenu();
      if(digitalRead(B4_PIN) == LOW) returnToMenu();
  }
}

void drawScreensaver() {
  lcd.clear();
  
  // Анимированный корабль
  lcd.setCursor(animPos, 0);
  lcd.write(byte(0));
  
  // Бегущая строка
  static byte textPos = 0;
  String text = "Arduino UNIX OS ";
  int len = text.length();
  String displayText = text.substring(textPos) + text + text;
  lcd.setCursor(0, 1);
  lcd.print(displayText.substring(0, 16));
  
  textPos = (textPos + 1) % len;
}

void handleMenuButtons() {
  if(digitalRead(B1_PIN) == LOW) {
    menuPos = (menuPos - 1 + 4) % 4;
    refreshDisplay = true;
  }
  if(digitalRead(B2_PIN) == LOW) {
    menuPos = (menuPos + 1) % 4;
    refreshDisplay = true;
  }
  if(digitalRead(B3_PIN) == LOW) {
    executeMenu();
    refreshDisplay = true;
  }
  if(digitalRead(B4_PIN) == LOW) {
    returnToMenu();
  }
}

void handleJumperButtons() {
  if(digitalRead(B3_PIN) == LOW) {
    if(!isJumping && jumpPhase == 0) {
      isJumping = true;
      jumpPhase = 1;
    }
    refreshDisplay = true;
  }
}

void handleClickerButtons() {
  if(digitalRead(B3_PIN) == LOW) { // Клик
    hackCount += clickPower;
    lastClickTime = millis();
    refreshDisplay = true;
  }
  
  if(digitalRead(B1_PIN) == LOW) { // Купить улучшение
    if(hackCount >= upgradeCost) {
      hackCount -= upgradeCost;
      clickPower *= 2;
      upgradeCost *= 3;
      refreshDisplay = true;
    }
  }
}

void returnToMenu() {
  if(currentState == GAME_JUMPER) gameActive = false;
  currentState = MAIN_MENU;
  refreshDisplay = true;
}

void drawMenu() {
  lcd.clear();
  lcd.print(">");
  lcd.print(menuItems[menuPos]);
  
  if(menuPos < 3) {
    lcd.setCursor(0, 1);
    lcd.print(" ");
    lcd.print(menuItems[(menuPos + 1) % 4]);
  }
}

void drawTerminal() {
  lcd.clear();
  lcd.print("Serial Terminal");
  lcd.setCursor(0, 1);
  lcd.print("Ready");
}

void processCommand(String cmd) {
  cmd.trim();
  int spaceIndex = cmd.indexOf(' ');
  String mainCmd = cmd;
  String args = "";
  
  if(spaceIndex > 0) {
    mainCmd = cmd.substring(0, spaceIndex);
    args = cmd.substring(spaceIndex + 1);
  }
  
 mainCmd.toLowerCase(); // Теперь присваиваем результат
  
  Serial.print("$ ");
  Serial.println(cmd);

  if(mainCmd == "help") {
    Serial.println("Available commands:");
    Serial.println("help      - Show this help");
    Serial.println("menu      - Return to menu");
    Serial.println("clear     - Clear screen");
    Serial.println("about     - System info");
    Serial.println("inofetch  - System summary");
    Serial.println("print [msg] - Print message");
  }
  else if(mainCmd == "clear") {
    Serial.write(12); // ASCII clear screen
  }
  else if(mainCmd == "about") {
    Serial.println("Arduino UNIX v0.5");
    Serial.println("With Hacky Hack");
  }
  else if(mainCmd == "inofetch") {
    printNeofetch();
  }
  else if(mainCmd == "print") {
    Serial.println(args);
  }
  else if(mainCmd == "menu") {
    currentState = MAIN_MENU;
  }
  else {
    Serial.print("Command not found: ");
    Serial.println(cmd);
  }
}

void printNeofetch() {
  Serial.print("     ██████            ██████           \n");
  Serial.print("  ████████████      ████████████        \n");
  Serial.print(" █████      ████████████     █████      \n");
  Serial.print("███          ████████   ████    ███     \n");
  Serial.print("███  ██████   ██████   ██████   ███     \n");
  Serial.print("███           ██████    ████    ███     \n");
  Serial.print("████        ████ █████         ████     \n");
  Serial.print(" ██████████████    ██████████████       \n");
  Serial.print("   ██████████        ██████████          \n");
  Serial.print("OS: Arduino UNIX 0.5\n");
  Serial.print("RAM: ");
  Serial.print(freeMemory());
  Serial.println(" bytes");
  Serial.print("Hacks: ");
  Serial.print(hackCount);
  Serial.println(" cracks");
}

void executeMenu() {
  switch(menuPos) {
    case 0: // Terminal
      currentState = APP_TERMINAL;
      lcd.clear();
      lcd.print("Terminal ready");
      delay(500);
      break;
      
    case 1: // Doggy Jumper
      currentState = GAME_JUMPER;
      initJumperGame();
      gameActive = true;
      break;
      
    case 2: // Hacky Hack
      currentState = GAME_CLICKER;
      initClickerGame();
      break;
      
    case 3: // System Info
      lcd.clear();
      lcd.print("RAM: ");
      lcd.print(freeMemory());
      lcd.print(" bytes");
      delay(2000);
      break;
  }
}

// Логика Doggy Jumper
void initJumperGame() {
  isJumping = false;
  jumpPhase = 0;
  obstaclePos = 16;
  score = 0;
  gameSpeed = 350;
  randomSeed(analogRead(0));
  obstacleType = random(2);
}

void gameJumperLogic() {
  if(millis() - lastUpdate > gameSpeed) {
    obstaclePos--;
    
    if(obstaclePos < -1) {
      obstaclePos = 16;
      score++;
      obstacleType = random(2);
      if(score % 3 == 0 && gameSpeed > 150) gameSpeed -= 20;
    }
    
    if(isJumping) {
      if(jumpPhase == 1) {
        jumpPhase = 2;
      } 
      else if(jumpPhase == 2) {
        jumpPhase = 0;
        isJumping = false;
      }
    }
    
    if(obstaclePos == 3) {
      if((obstacleType == 0 && jumpPhase != 2) ||
         (obstacleType == 1 && jumpPhase == 2)) {
        gameOver();
      }
    }
    
    refreshDisplay = true;
    lastUpdate = millis();
  }
}

void drawJumperGame() {
  lcd.clear();
  lcd.setCursor(2, 0);
  if(jumpPhase == 2) lcd.print('@');
  lcd.setCursor(2, 1);
  if(jumpPhase != 2) lcd.print('@');
  lcd.setCursor(obstaclePos, obstacleType);
  lcd.print(obstacleType == 0 ? 'X' : '^');
  lcd.setCursor(12, 0);
  lcd.print("S:");
  lcd.print(score);
}

// Логика Hacky Hack
void initClickerGame() {
  hackCount = 0;
  clickPower = 1;
  upgradeCost = 10;
  refreshDisplay = true;
}

void drawClickerGame() {
  lcd.clear();
  lcd.print("Hacks: ");
  lcd.print(hackCount);
  
  lcd.setCursor(0, 1);
  lcd.print("Upgrade: ");
  lcd.print(upgradeCost);
}

void gameOver() {
  gameActive = false;
  lcd.clear();
  lcd.print("Game Over!");
  lcd.setCursor(0, 1);
  lcd.print("Score: ");
  lcd.print(score);
  delay(2000);
  currentState = MAIN_MENU;
}

int freeMemory() {
  extern int __heap_start, *__brkval; 
  int v; 
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval); 
}