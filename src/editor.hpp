#ifndef EDITOR_HPP
#define EDITOR_HPP

#include "fileEditor.hpp"
#include <string>
#include <ncurses.h>
#include <algorithm>

class Editor {
public:
	Editor(const std::string& filePath, int tabSize = 4);
	
	bool close();
	
	void draw();
	int getInput();
	
	void put(char ch);

	inline void setScrollH(int val) {
		scrollX = std::clamp(val, 0, file.getLineSize() - 1);
	}
	inline void setScrollV(int val) {
		scrollY = std::clamp(val, 0, file.linesAmount() - 1);
	}
	inline void scrollH(int amount) { setScrollH(scrollX + amount); }
	inline void scrollV(int amount) { setScrollV(scrollY + amount); }
	inline void scrollUp(int amount = 1) { scrollV(-amount); }
	inline void scrollDown(int amount = 1) { scrollV(amount); }
	inline void scrollRight(int amount = 1) { scrollH(amount); }
	inline void scrollLeft(int amount = 1) { scrollH(-amount); }

	void moveUp();
	void moveDown();
	void moveLeft();
	void moveRight();
	void moveBeginningOfLine();
	void moveEndOfLine();
    void moveBeginningOfText();
    void moveEndOfText();

	void saveFile();
    void newLine();
	void deleteCharL();
    void deleteCharR();
	
	void setStatus(const std::string& message);
	void setStatus(const std::string& message, int colorPair);
	void resetStatus();

	void initColorPairs() const;
	void applyColorPairToStatusBar(int colorPair);
	
    inline bool isAlive() const {
        return alive;
    }
	inline int getOnScreenCursorX() const {
		return getcurx(stdscr);
	}
	inline int getOnScreenCursorY() const {
		return getcury(stdscr);
	}
	inline int getTextEditorWidth() const {
		return width;
	}
	inline int getTextEditorHeight() const {
		return height;
	}
	
private:
	FileEditor file;
	
	const int TAB_SIZE;
	
	int scrollX;
	int scrollY;
	int width;
	int height;

    bool alive;

	std::string statusText;
	bool customStatusText{false};

	// Color control variable:
	int colorPair{1};
	
	Caret caret;
	
	inline int getVirtualCaretColumnToCaret() {
		return getVirtualCaretColumnToCaret(file.getCaretY());
	}
	
	inline int getVirtualCaretColumnToCaret(int row) {
		int size{};
		const std::string& line = file.getLine(row);
		for (int col = 0; col < file.getCaretX(); col++) {
			if(line[col] != '\t') {
				size++;
			} else {
				size += TAB_SIZE - (size) % TAB_SIZE;
			}
		}
		return size;
	}
	
	inline int getVirtualCaretColumn(int x, int y) {
		int size{};
		const std::string& line = file.getLine(y);
		for (int col = 0; col < x; col++) {
			if(line[col] != '\t') {
				size++;
			} else {
				size += TAB_SIZE - (size) % TAB_SIZE;
			}
		}
		return size;
	}
	
	inline int getVirtualLineLength() {
		return getVirtualLineLength(caret.y);
	}
	inline int getVirtualLineLength(int y) {
		return getVirtualCaretColumn(file.getLineSize(), y);
	}
	
	inline int getFileCaretColumn() {
		return getFileCaretColumn(getVirtualCaretColumnToCaret());
	}
	inline int getFileCaretColumn(int virtualColumn) {
		int size{};
		const std::string& line = file.getLine(caret.y);
		for (int col = 0; col < virtualColumn;) {
			if(line[size] == '\t') {
				col += TAB_SIZE - (col) % TAB_SIZE;
			} else {
				col++;
			}
			if(col <= virtualColumn) {
				size++;
			}
		}
		return size;
	}
	inline void setCaretLocation(int x, int y) {
		caret.y = std::clamp(y, 0, (int)file.linesAmount() - 1);
		caret.x = caret.savedX = std::clamp(x, 0, (int)getVirtualLineLength(caret.y));
		file.setCaretLocation(getFileCaretColumn(caret.x), caret.y);
		
		scrollToCaret();
	}
	
	inline void scrollToCaret() {
		if(caret.x < scrollX) {
			scrollLeft((scrollX) - (caret.x));
		}
		if(caret.x > getTextEditorWidth() - 1 + scrollX) {
			scrollRight((caret.x) - (getTextEditorWidth() - 1 + scrollX));
		}
		// if(caret.x > getTextEditorWidth() + scrollX || caret.x < getTextEditorWidth() + scrollX) {
		// 	scrollH(caret.x - (getTextEditorWidth() + scrollX) + 2);
		// }
		if(caret.y < scrollY) {
			scrollUp((scrollY) - (caret.y));
		}
		if(caret.y > getTextEditorHeight() - 1 + scrollY) {
			scrollDown((caret.y) - (getTextEditorHeight() - 1 + scrollY));
		}
	}
};

#endif
