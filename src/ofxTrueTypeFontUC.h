#pragma once

#include <vector>
#include "ofRectangle.h"
#include "ofPath.h"

class ofxTrueTypeFontUC {
	
public:
	ofxTrueTypeFontUC();
	virtual ~ofxTrueTypeFontUC();
  
	//set the default dpi for all typefaces.
	static void setGlobalDpi(int newDpi);
  
	// 			-- default (without dpi), anti aliased, 96 dpi:
	bool loadFont(string filename, int fontsize, bool bAntiAliased=true, bool makeContours=false, float simplifyAmt=0.3, int dpi=0);
  void reloadFont();
  void unloadFont();
  
	void drawString(wstring s, float x, float y);
  void drawString(string s, float x, float y);
	void drawStringAsShapes(wstring s, float x, float y);
  void drawStringAsShapes(string s, float x, float y);
  
  ofPath getCharacterAsPoints(wstring character);
  ofPath getCharacterAsPoints(string character);
	vector<ofPath> getStringAsPoints(wstring s);
	vector<ofPath> getStringAsPoints(string s);
	
	bool isLoaded();
	bool isAntiAliased();
	
	int getFontSize();
  
	float getLineHeight();
	void setLineHeight(float height);
  
	float getLetterSpacing();
	void setLetterSpacing(float spacing);
  
	float getSpaceSize();
	void setSpaceSize(float size);
  
	float stringWidth(wstring s);
  float stringWidth(string s);
  
	float stringHeight(wstring s);
  float stringHeight(string s);
	
	ofRectangle getStringBoundingBox(wstring s, float x, float y);
  ofRectangle getStringBoundingBox(string s, float x, float y);
	
	int	getLoadedCharactersCount();
	int getLimitCharactersNum();
	void reserveCharacters(int charactersNumber);
  
private:
  class Impl;
  Impl *mImpl;
  
  // disallow copy and assign
  ofxTrueTypeFontUC(const ofxTrueTypeFontUC &);
  void operator=(const ofxTrueTypeFontUC &);
};


namespace util {
  namespace ofxTrueTypeFontUC {
    
    wstring convToWString(string src);
    
  } // ofxTrueTypeFontUC
} // util
