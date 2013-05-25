#ofxTrueTypeFontUC#
ofxTrueTypeFontUC is an extension of ofTrueTypeFont class for UNICODE characters.

##License##
ofxTrueTypeFontUC is distributed under the MIT License. This gives everyone the freedoms to use ofxTrueTypeFontUC  in any context: commercial or non-commercial, public or private, open or closed source.

The MIT License (MIT)

Copyright (c) 2011- hironishihara

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

##Methods##
###bool loadFont(…)###
bool loadFont(string filename, int fontsize, bool bAntiAliased, bool makeContours, float simplifyAmt, int dpi)  
  
###void drawString(...)###
void drawString(wstring ws, float x, float y)  
void drawString(string s, float x, float y)  

###void drawStringAsShapes(...)###
void drawStringAsShapes(wstring ws, float x, float y)  
void drawStringAsShapes(string s, float x, float y)  
  
###ofPath getCharacterAsPoints(...)###
ofPath getCharacterAsPoints(wstring wcharacter)  
ofPath getCharacterAsPoints(string character)  
  
###vector&lt;ofPath> getStringAsPoints(…)###
vector&lt;ofPath> getStringAsPoints(wstring ws)  
vector&lt;ofPath> getStringAsPoints(string s)  
  
###bool isLoaded()###
bool isLoaded()  

###bool isAntiAliased()###
bool isAntiAliased()  
  
###int getFontSize()###
int getFontSize()  
  
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
float stringWidth(wstring ws)  
float stringWidth(string s)  
  
###float stringHeight(...)###
float stringHeight(wstring ws)  
float stringHeight(string s)  
  
###ofRectangle getStringBoundingBox(...)###
ofRectangle getStringBoundingBox(wstring ws, float x, float y)   
ofRectangle getStringBoundingBox(string s, float x, float y)  

###void reloadFont()###
void reloadFont()  

###void unloadFont()###
void unloadFont()  

###int getLoadedCharactersCount()###
int  getLoadedCharactersCount()
  
###int getLimitCharactersNum()###
int getLimitCharactersNum()  

###void reserveCharacters(...)###
void reserveCharacters(int charactersNumber)  
  
##Additional Methods##
###wstring util::ofxTrueTypeFontUC::convToWString(...)###
wstring util::ofxTrueTypeFontUC::convToWString(string src)  
  
