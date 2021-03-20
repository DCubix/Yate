#include "editor.hpp"
#include <ncurses.h>
#include <unordered_map>

// Color pairs defines:
#define PAIR_STANDARD 1
#define PAIR_ERROR 2
#define PAIR_WARNING 3
#define PAIR_INFO 4

Editor::Editor(const std::string& filePath, int tabSize) 
	: file(filePath), 
	TAB_SIZE(tabSize),
	scrollX(0),
	scrollY(0),
	width(getmaxx(stdscr) - 4), 
	height(getmaxy(stdscr) - 2),
    alive(true),
	caret() {
	resetStatus();
	if (!file.hasWritePermission()) {
		setStatus(" File \'" + file.getFullFilename() + "\' doesn't have write permissions. ", PAIR_WARNING);
	}
	initColorPairs();
}

void Editor::draw() {
	clear();
	for (int lineNr = scrollY; lineNr < scrollY + height && lineNr < file.linesAmount(); lineNr++) {
		std::string line { file.getLine(lineNr) };
		int min = getTextEditorWidth();
		int virtualCol = 0;
		
		move(lineNr - scrollY, 0);
		printw("%3d ", lineNr + 1);
		for (char ch : line) {
			if(ch == '\t') {
				const int tabSize = TAB_SIZE - (virtualCol) % TAB_SIZE;
				for(int original = virtualCol; virtualCol < original + tabSize; virtualCol++) {
					if(virtualCol >= scrollX && virtualCol - scrollX <= min) {
						printw(" ");
					}
				}
			} else {
				if(virtualCol >= scrollX && virtualCol - scrollX  <= min) {
					printw("%c", ch);
				}
				virtualCol++;
			}
		}
	}

	// turn on and set color for status
	attron(A_STANDOUT);
	attron(COLOR_PAIR(this->colorPair));
	
	// print status at bottom of screen
	mvprintw(getmaxy(stdscr) - 1, 0, this->statusText.c_str());

	attroff(COLOR_PAIR(this->colorPair));
	// Reset color pair:
	this->colorPair = PAIR_STANDARD;
	attroff(A_STANDOUT);

	// std::string cursorText = file.getLine(file.getCaretY()).substr(0, file.getCaretX());
	// mvprintw(file.getCaretY() - scrollY, 4, cursorText.c_str());
	
	move(caret.y - scrollY, caret.x - scrollX + 4);
	refresh();
}

void Editor::getInput() {
	int input = getch();

	if((input >= 32 && input < 127) || input == KEY_STAB || input == 9) {
		put(static_cast<char>(input));
		if(!file.hasWritePermission()) {
			setStatus(" Warning: File \'" + file.getFullFilename() + "\' doesn't have write permissions. ", PAIR_WARNING);
		}
	}
	else
	{
		switch(input)
		{
			case 22:
			setCaretLocation(caret.x, caret.y - getTextEditorHeight() - 1);
				break;
			case 2:
			setCaretLocation(caret.x, caret.y + getTextEditorHeight() - 1);
				break;
			case 13:
			scrollLeft();
				break;
			case 14:
			scrollRight();
				break;
			case KEY_UP:
				moveUp();
				break;
			case KEY_DOWN:
				moveDown();
				break;
			case KEY_LEFT:
				moveLeft();
				break;
			case KEY_RIGHT:
				moveRight();
				break;
			case 5:
			case KEY_END:
				moveEndOfLine(); 
				break;
			case KEY_HOME:
			case 1:
				moveBeginningOfLine();
				break;
			case 25: // CTRL+Y (for qwertz layout)
			case 26: // CTRL+Z (for qwerty layout)
				moveBeginningOfText(); // TODO: FIX
				break;
			case 24: // CTRL+X
				moveEndOfText(); // TODO: FIX
				break;
			case KEY_ENTER:
			case 10:
				newLine();
				break;
			case KEY_BACKSPACE:
			case 127:
				deleteCharL();
				break;
			// NOTE: let's do both KEY_DL and KEY_DC to be safe (famous last words)
			case 330:
			case KEY_DL:
				deleteCharR();
				break;
			case 19:
				saveFile();
				break;
			case 3:
				file.close();
				// NOTE: Maybe instead of exiting without saving, ask the user if he wants to save
				endwin();
				exit(0);
				break;
		}
	}

	// Reset status on user input if no custom status was applied, if there is a custom status, let it display first and then reset
	if(!customStatusText) {
		resetStatus();
	}
	customStatusText = false;
	
#ifndef NDEBUG
		setStatus(this->statusText + "\tinput: " + std::to_string(input), this->colorPair);
		customStatusText = false;
#endif
}

void Editor::put(char ch) {
	file.put(ch);
	moveRight();
}


// void Editor::scrollH(int amount) {
// 	scrollX += std::clamp(scrollX + amount, 0, file.getLineSize() - 1);
// }
// void Editor::scrollV(int amount) {
// 	scrollY += std::clamp(scrollY + amount, 0, file.linesAmount() - 1);
// }
// void Editor::scrollUp(int amount) {
// 	scrollY -= std::clamp(scrollY + amount, 0, file.linesAmount() - 1);
// }
// void Editor::scrollDown(int amount) {
// 	scrollY += std::clamp(scrollY + amount, 0, file.linesAmount() - 1);
// }
// void Editor::scrollRight(int amount) {
// 	scrollX += std::clamp(scrollX + amount, 0, file.getLineSize() - 1);
// }
// void Editor::scrollLeft(int amount) {
// 	scrollX -= std::clamp(scrollX + amount, 0, file.getLineSize() - 1);
// }

void Editor::moveUp() {
	if(scrollY - file.getCaretY() >= 0)
		scrollUp();
		
	if(caret.y - 1 >= 0) {
		caret.y--;
		file.moveUp();
		if (getVirtualLineLength() < caret.savedX) {
			caret.x = getVirtualLineLength();
			file.setCaretLocation(file.getLineSize(), file.getCaretY());
		} else {
			file.setCaretLocation(getFileCaretColumn(caret.savedX), file.getCaretY());
			caret.x = getVirtualCaretColumn(file.getCaretX(), file.getCaretY());
		} 
	} else {
		caret.y = 0;
		caret.x = caret.savedX = 0;
		file.setCaretLocation(0, 0);
	}
	scrollToCaret();
}
void Editor::moveDown() {
	if((scrollY + height) - caret.y - 1 <= 0 && caret.y + 1 < file.linesAmount()) {
		scrollDown();
	}
	
	if(caret.y + 1 < file.linesAmount()) {
		caret.y++;
		file.moveDown();
		if (getVirtualLineLength() < caret.savedX) {
			caret.x = getVirtualLineLength();
			file.setCaretLocation(file.getLineSize(), file.getCaretY());
		} else {
			file.setCaretLocation(getFileCaretColumn(caret.savedX), file.getCaretY());
			caret.x = getVirtualCaretColumn(file.getCaretX(), file.getCaretY());
		} 
	} else {
		file.setCaretLocation(file.getLineSize(), file.linesAmount() - 1);
		caret.y = file.linesAmount() - 1;
		caret.x = caret.savedX = getVirtualCaretColumn(file.getLineSize(), file.linesAmount() - 1);
	}
	scrollToCaret();
}
void Editor::moveRight() {
	int prev = caret.x;
	if (caret.x < getVirtualCaretColumn(file.getLineSize(caret.y), caret.y)) {
		file.moveRight();
		caret.x = caret.savedX = getVirtualCaretColumn(getFileCaretColumn(), caret.y);
	} else if (caret.y < file.linesAmount() - 1) {
		moveDown();
		setScrollH(0);
		caret.x = caret.savedX = 0;
		file.setCaretLocation(0, file.getCaretY());
	}
	
	if(caret.x - scrollX + 1 > getTextEditorWidth()) 
		scrollRight(caret.x - prev); // NOTE: calculating difference in case if there is a tab
}
void Editor::moveLeft() {
	int prev = caret.x;
	if(caret.x > 0) {
		file.moveLeft();
		caret.x = caret.savedX = getVirtualCaretColumn(getFileCaretColumn(), caret.y);
	} else if(caret.y > 0) {
		moveUp();
		int virtualX = getVirtualCaretColumn(file.getLineSize(caret.y), caret.y);
		setScrollH(virtualX - getTextEditorWidth() + 2);
		caret.x = caret.savedX = virtualX;
		file.setCaretLocation(file.getLineSize(caret.y), file.getCaretY());
	}
	
	if(caret.x - scrollX < 0)
		scrollLeft(prev - caret.x); // NOTE: calculating difference in case if there is a tab
}

void Editor::moveBeginningOfLine() {
	setCaretLocation(0, caret.y);
}
void Editor::moveEndOfLine() {
	setCaretLocation(getVirtualLineLength(), caret.y);
}
void Editor::moveBeginningOfText() {
	if(caret.x == 0 && caret.y == 0) return;

	moveLeft();
	while(file.getLine()[file.getCaretX() - 1] != ' ' && file.getLine()[file.getCaretX() - 1] != '\t' && file.getCaretX() != 0) {
		moveLeft();
	}
}
void Editor::moveEndOfText() {
	if(caret.x == getVirtualLineLength() && caret.y == file.linesAmount()) return;

	moveRight();
	while(file.getLine()[file.getCaretX()] != ' ' && file.getLine()[file.getCaretX()] != '\t' && file.getCaretX() != file.getLineSize()) {
		moveRight();
	}
}
void Editor::newLine() {
	file.newLine();
    moveDown();
	setCaretLocation(0, caret.y);
}
void Editor::deleteCharL() {
	try {
		if(caret.x == 0) {
			moveLeft();
			file.del(true);
		} else {
			file.del(false);
			moveLeft();
		}
    	if(file.getCaretY() - scrollY < 0) {
        	scrollY--;
    	}
		caret.savedX = caret.x;
	} catch(std::string e) {
		setStatus(e, PAIR_ERROR);
	}
}
void Editor::deleteCharR() {
	try {
		file.del(true);
		caret.savedX = caret.x;
	} catch(std::string e) {
		setStatus(e, PAIR_ERROR);
	}
}

void Editor::saveFile() {
	if (file.hasWritePermission()) {
		setStatus(" File \'" + file.getFullFilename() + "\' has been saved. ", PAIR_INFO);
		file.save();
	} else {
		setStatus(" File \'" + file.getFullFilename() + "\' doesn't have write permissions. ", PAIR_ERROR);
	}
}

void Editor::setStatus(const std::string& message) {
	setStatus(message, PAIR_STANDARD);
}
void Editor::setStatus(const std::string& message, int colorPair) {
	this->statusText = message;
	this->colorPair = colorPair;
	customStatusText = true;
}
void Editor::resetStatus() {
	char buffer[256];
	sprintf(
		buffer, 
		" File: %s\tc.x %2d, c.y %2d, c.sx %2d | f.x %2d, f.y %2d", 
		file.getFullFilename().c_str(), 
		caret.x, caret.y, caret.savedX, 
		file.getCaretX(), file.getCaretY()
	);
	setStatus(buffer);
}

// Defines color pairs
void Editor::initColorPairs() const {
	init_pair(PAIR_ERROR, COLOR_WHITE, COLOR_RED);
	init_pair(PAIR_STANDARD, COLOR_WHITE, COLOR_BLACK);
	init_pair(PAIR_WARNING, COLOR_WHITE, COLOR_RED);
	init_pair(PAIR_INFO, COLOR_WHITE, COLOR_BLUE);
}
