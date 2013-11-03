#pragma once


#include <vector>
#include "ofRectangle.h"
#include "ofPath.h"

//--------------------------------------------------
const static string OF_TTFUC_SANS = "sans-serif";
const static string OF_TTFUC_SERIF = "serif";
const static string OF_TTFUC_MONO = "monospace";

//--------------------------------------------------
class ofxTrueTypeFontUC{
public:
	ofxTrueTypeFontUC();
	virtual ~ofxTrueTypeFontUC();

	// -- default (without dpi), anti aliased, 96 dpi:
	bool loadFont(const string &filename, int fontsize, bool bAntiAliased=true, bool makeContours=false, float simplifyAmt=0.3, int dpi=0);
  void reloadFont();
  
  void drawString(const string &utf8_string, float x, float y);
	void drawStringAsShapes(const string &utf8_string, float x, float y);
  
  vector<ofPath> getStringAsPoints(const string &utf8_string, bool vflip=ofIsVFlipped());
	ofRectangle getStringBoundingBox(const string &utf8_string, float x, float y);
	
	bool isLoaded();
	bool isAntiAliased();
  
  int getSize();
  float getLineHeight();
  void setLineHeight(float height);
	float getLetterSpacing();
	void setLetterSpacing(float spacing);
	float getSpaceSize();
	void setSpaceSize(float size);
	float stringWidth(const string &utf8_string);
	float stringHeight(const string &utf8_string);
  // get the num of loaded chars
	int getNumCharacters();
	ofTextEncoding getEncoding() const;
	// set the default dpi for all typefaces
	static void setGlobalDpi(int newDpi);
  
private:
  class Impl;
  Impl *mImpl;
  
  // disallow copy and assign
  ofxTrueTypeFontUC(const ofxTrueTypeFontUC &);
  void operator=(const ofxTrueTypeFontUC &);
};

//--------------------------------------------------
namespace util {
  namespace ofxTrueTypeFontUC {
    basic_string<unsigned int> convUTF8ToUTF32(const string & utf8_string);
  }
}

