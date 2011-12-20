#pragma once


#include <vector>
#include "ofPoint.h"
#include "ofRectangle.h"
#include "ofConstants.h"
#include "ofPath.h"
#include "ofTexture.h"
#include "ofMesh.h"

//--------------------------------------------------
typedef struct {
	int character;
	int height;
	int width;
	int setWidth;
	int topExtent;
	int leftExtent;
	float tW,tH;
	float x1,x2,y1,y2;
	float t1,t2,v1,v2;
} charPropsUC;


typedef ofPath ofTTFCharacterUC;

//--------------------------------------------------
#define NUM_CHARACTER_TO_START_UC		33		// 0 - 32 are control characters, no graphics needed.

class ofxTrueTypeFontUC{
	
public:
	ofxTrueTypeFontUC();
	virtual ~ofxTrueTypeFontUC();
	
	//set the default dpi for all typefaces.
	static void setGlobalDpi(int newDpi);
	
	// 			-- default (without dpi), anti aliased, 96 dpi:
	bool loadFont(string filename, int fontsize, bool _bAntiAliased=true, bool makeContours=false, float _simplifyAmt=0.3, int dpi=0);
	
	bool isLoaded();
	bool isAntiAliased();
	bool hasFullCharacterSet();
	
	int getSize();
	float getLineHeight();
	void setLineHeight(float height);
	float getLetterSpacing();
	void setLetterSpacing(float spacing);
	float getSpaceSize();
	void setSpaceSize(float size);
	float stringWidth(wstring s);
	float stringHeight(wstring s);
	
	ofRectangle getStringBoundingBox(wstring s, float x, float y);
	
	void drawString(wstring s, float x, float y);
	void drawStringAsShapes(wstring s, float x, float y);
	
	// get the num chars in the loaded char set
	int	getNumCharacters();	
	
	ofTTFCharacterUC getCharacterAsPoints(wchar_t character);
	vector<ofTTFCharacterUC> getStringAsPoints(wstring s);

	int getNumMaxCharacters();
	void setNumMaxCharacters(int num);
	
protected:
	bool bLoadedOk;
	bool bAntiAliased;
	static const bool bFullCharacterSet;
	int nMaxCharacters;
	float simplifyAmt;
	
	vector <ofTTFCharacterUC> charOutlines;
	
	float lineHeight;
	float letterSpacing;
	float	spaceSize;
	
	vector<charPropsUC> cps;			// properties for each character
	
	int	fontSize;
	bool bMakeContours;
	
	void drawChar(int c, float x, float y);
	void drawCharAsShape(int c, float x, float y);
	
	int	border;//, visibleBorder;
	string filename;
	
	// ofTexture texAtlas;
	vector<ofTexture> textures;
	bool binded;
	ofMesh stringQuads;
	
private:
	typedef struct FT_LibraryRec_  *FT_Library;
	typedef struct FT_FaceRec_*  FT_Face;
	FT_Library library;
	FT_Face face;
  
  ofTTFCharacterUC getCharacterAsPointsFromCharID(const int &charID);
  
  void bind(const int &charID);
	void unbind(const int &charID);
	
	int getCharID(const int &c);
	void loadChar(const int &charID);
	vector<int> loadedChars;
	static const int TYPEFACE_UNLOADED;
	static const int DEFAULT_NUM_MAX_CHARACTERS;
	
#ifdef TARGET_ANDROID
	friend void ofUnloadAllFontTextures();
	friend void ofReloadAllFontTextures();
#endif
#ifdef TARGET_OPENGLES
	GLint blend_src, blend_dst;
	GLboolean blend_enabled;
	GLboolean texture_2d_enabled;
#endif
	void unloadTextures();
	void reloadTextures();
};


