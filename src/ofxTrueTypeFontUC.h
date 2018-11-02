#pragma once

#include <vector>
#include "ofMain.h"

//--------------------------------------------------
const static std::string OF_TTFUC_SANS = "sans-serif";
const static std::string OF_TTFUC_SERIF = "serif";
const static std::string OF_TTFUC_MONO = "monospace";

//--------------------------------------------------

class ofxTrueTypeFontUC{
  
public:
  ofxTrueTypeFontUC();
  virtual ~ofxTrueTypeFontUC();
  
  //set the default dpi for all typefaces.
  static void setGlobalDpi(int newDpi);
  
  // 			-- default (without dpi), anti aliased, 96 dpi:
  bool load(std::string filename, int fontsize, bool bAntiAliased=true, bool makeContours=false, float simplifyAmt=0.3, int dpi=0);
  bool loadFont(std::string filename, int fontsize, bool bAntiAliased=true, bool makeContours=false, float simplifyAmt=0.3, int dpi=0);
  void reloadFont();
  void unloadFont();
  
  void drawString(const std::string &str, float x, float y);
  void drawStringAsShapes(const std::string &str, float x, float y);
  
  vector<ofPath> getStringAsPoints(const std::string &str, bool vflip=ofIsVFlipped());
  ofRectangle getStringBoundingBox(const std::string &str, float x, float y);
  
  bool isLoaded();
  bool isAntiAliased();
  
  int getFontSize();
  
  float getLineHeight();
  void setLineHeight(float height);
  
  float getLetterSpacing();
  void setLetterSpacing(float spacing);
  
  float getSpaceSize();
  void setSpaceSize(float size);
  
  float stringWidth(const std::string &str);
  float stringHeight(const std::string &str);
  // get the num of loaded chars
  int getNumCharacters();
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
