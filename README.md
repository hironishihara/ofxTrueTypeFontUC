#ofxTrueTypeFontUC#
ofxTrueTypeFontUC is an extension of ofTrueTypeFont class for UNICODE characters. Current version is based on version 0.8.0.

##License##
ofxTrueTypeFontUC is distributed under the MIT License. This gives everyone the freedoms to use ofxTrueTypeFontUC  in any context: commercial or non-commercial, public or private, open or closed source.

Please see License.txt for details.

##Methods##
###bool loadFont(…)###
bool loadFont(const string &filename, int fontsize, bool bAntiAliased=true, bool makeContours=false, float simplifyAmt=0.3, int dpi=0)  

###void reloadFont()###
void reloadFont()  
  
###void drawString(...)###
void drawString(const string &utf8_string, float x, float y)  

###void drawStringAsShapes(...)###
void drawStringAsShapes(const string &utf8_string, float x, float y)  
In many cases, this function is faster than drawString(...).
  
###vector&lt;ofPath> getStringAsPoints(…)###
vector&lt;ofPath> getStringAsPoints(const string &utf8_string, bool vflipvflip=ofIsVFlipped())  

###ofRectangle getStringBoundingBox(...)###
ofRectangle getStringBoundingBox(const string &utf8_string, float x, float y)  
  
###bool isLoaded()###
bool isLoaded()  

###bool isAntiAliased()###
bool isAntiAliased()  
  
###int getSize()###
int getSize()  
  
###float getLineHeight()###
float getLineHeight()  

###void setLineHeight(...)###
void setLineHeight(float height)  
  
###float getLetterSpacing()###
float getLetterSpacing()  

###void setLetterSpacing(...)###
void setLetterSpacing(float spacing)  
  
###float getSpaceSize()###
float getSpaceSize()  

###void setSpaceSize(...)###
void setSpaceSize(float size)  
  
###float stringWidth(...)###
float stringWidth(string s)  
  
###float stringHeight(...)###
float stringHeight(string s)  
  
###int getNumCharacters()###
int getNumCharacters()

###ofTextEncoding getEncoding()###
ofTextEncoding getEncoding()

###void setGlobalDpi(...))###
void setGlobalDpi(int newDpi)
  
##Additional Methods##
###wstring util::ofxTrueTypeFontUC::convUTF8ToUTF32(...)###
wstring util::ofxTrueTypeFontUC::convUTF8ToUTF32(const string &utf8_string)  
  
