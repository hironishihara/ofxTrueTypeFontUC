#include "ofxTrueTypeFontUC.h"
//--------------------------

#include "ft2build.h"
#include "freetype2/freetype/freetype.h"
#include "freetype2/freetype/ftglyph.h"
#include "freetype2/freetype/ftoutln.h"
#include "freetype2/freetype/fttrigon.h"

#ifdef TARGET_LINUX
#include <fontconfig/fontconfig.h>
#endif

#include <algorithm>

#include "ofUtils.h"
#include "ofGraphics.h"
#include "ofAppRunner.h"
#include "ofPoint.h"
#include "ofConstants.h"
#include "ofTexture.h"
#include "ofMesh.h"
#include "Poco/TextConverter.h"
#include "Poco/UTF8Encoding.h"


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

static bool printVectorInfo = false;
static int ttfGlobalDpi = 96;
static bool librariesInitialized = false;

typedef struct FT_LibraryRec_ * FT_Library;
typedef struct FT_FaceRec_ * FT_Face;
static FT_Library library;
static FT_Face face;

//========================================================
class ofxTrueTypeFontUC::Impl {
public:
  Impl(){};
  ~Impl(){};
  
  bool bLoadedOk;
  bool bAntiAliased;
  float lineHeight;
  float letterSpacing;
  float spaceSize;
  int fontSize;
  bool bMakeContours;
  float simplifyAmt;
  int dpi;
  int border;
  string filename;
  bool binded;
  ofTextEncoding encoding;
  
  GLint blend_src, blend_dst;
  GLboolean blend_enabled;
  GLboolean texture_2d_enabled;
  ofMesh stringQuads;
  
  const unsigned int kLimitCharactersNum = 10000;
  const unsigned int kTypeFaceUnloaded = 0;
  
  vector<unsigned int> loadedChars;
  vector<charPropsUC> cps;  // properties for each character
  vector <ofPath> charOutlines;
  vector <ofPath> charOutlinesNonVFlipped;
  vector<ofTexture> texAtlas;
  
  void drawChar(const unsigned int &charID, float x, float y);
  void drawCharAsShape(const unsigned int &charID, float x, float y);
  ofPath getCharacterAsPoints(const unsigned int &charID, bool vflip=ofIsVFlipped());
  
  unsigned int getCharID(const unsigned int &);
  bool loadChar(const unsigned int &);
  void bind(const unsigned int &);
  void unbind(const unsigned int &);
  
#if defined(TARGET_ANDROID) || defined(TARGET_OF_IOS)
  void ofUnloadAllFontTextures();
  void ofReloadAllFontTextures();
#endif
  
  void unloadTextures();
  static bool initLibraries();
  static void finishLibraries();
};

//--------------------------------------------------------
void ofxTrueTypeFontUC::setGlobalDpi(int newDpi){
  ttfGlobalDpi = newDpi;
}

//--------------------------------------------------------
static ofPath makeContoursForCharacter(FT_Face &face);
static ofPath makeContoursForCharacter(FT_Face &face){
  int nContours = face->glyph->outline.n_contours;
  int startPos = 0;
  
  char * tags = face->glyph->outline.tags;
  FT_Vector * vec = face->glyph->outline.points;
  
  ofPath charOutlines;
  charOutlines.setUseShapeColor(false);
  
  for (int k = 0; k < nContours; k++) {
    if ( k > 0 ) {
      startPos = face->glyph->outline.contours[k-1]+1;
    }
    int endPos = face->glyph->outline.contours[k]+1;
    
    if (printVectorInfo) {
      ofLogNotice("ofxTrueTypeFontUC") << "--NEW CONTOUR";
    }
    
    //vector <ofPoint> testOutline;
    ofPoint lastPoint;
    
    for (int j = startPos; j < endPos; j++) {
      
      if ( FT_CURVE_TAG(tags[j]) == FT_CURVE_TAG_ON ) {
        lastPoint.set((float)vec[j].x, (float)-vec[j].y, 0);
        if (printVectorInfo) {
          ofLogNotice("ofxTrueTypeFontUC") << "flag[" << j << "] is set to 1 - regular point - " << lastPoint.x <<  lastPoint.y;
        }
        charOutlines.lineTo(lastPoint/64);
        
      }
      else {
        if (printVectorInfo) {
          ofLogNotice("ofxTrueTypeFontUC") << "flag[" << j << "] is set to 0 - control point";
        }
        
        if (FT_CURVE_TAG(tags[j]) == FT_CURVE_TAG_CUBIC) {
          if (printVectorInfo) {
            ofLogNotice("ofxTrueTypeFontUC") << "- bit 2 is set to 2 - CUBIC";
          }
          
          int prevPoint = j-1;
          if ( j == 0) {
            prevPoint = endPos-1;
          }
          
          int nextIndex = j+1;
          if ( nextIndex >= endPos) {
            nextIndex = startPos;
          }
          
          ofPoint nextPoint((float)vec[nextIndex].x, -(float)vec[nextIndex].y);
          
          //we need two control points to draw a cubic bezier
          bool lastPointCubic =  (FT_CURVE_TAG(tags[prevPoint]) != FT_CURVE_TAG_ON) && (FT_CURVE_TAG(tags[prevPoint]) == FT_CURVE_TAG_CUBIC);
          
          if (lastPointCubic) {
            ofPoint controlPoint1((float)vec[prevPoint].x, (float)-vec[prevPoint].y);
            ofPoint controlPoint2((float)vec[j].x, (float)-vec[j].y);
            ofPoint nextPoint((float) vec[nextIndex].x, -(float) vec[nextIndex].y);
            
            //cubic_bezier(testOutline, lastPoint.x, lastPoint.y, controlPoint1.x, controlPoint1.y, controlPoint2.x, controlPoint2.y, nextPoint.x, nextPoint.y, 8);
            charOutlines.bezierTo(controlPoint1.x/64, controlPoint1.y/64, controlPoint2.x/64, controlPoint2.y/64, nextPoint.x/64, nextPoint.y/64);
          }
        }
        else {
          ofPoint conicPoint((float)vec[j].x, -(float)vec[j].y);
          
          if (printVectorInfo) {
            ofLogNotice("ofxTrueTypeFontUC") << "- bit 2 is set to 0 - conic- ";
            ofLogNotice("ofxTrueTypeFontUC") << "--- conicPoint point is " << conicPoint.x << conicPoint.y;
          }
          
          //If the first point is connic and the last point is connic then we need to create a virutal point which acts as a wrap around
          if (j == startPos) {
            bool prevIsConnic = (FT_CURVE_TAG( tags[endPos-1]) != FT_CURVE_TAG_ON) && (FT_CURVE_TAG( tags[endPos-1]) != FT_CURVE_TAG_CUBIC);
            
            if (prevIsConnic) {
              ofPoint lastConnic((float)vec[endPos - 1].x, (float)-vec[endPos - 1].y);
              lastPoint = (conicPoint + lastConnic) / 2;
              
              if (printVectorInfo) {
                ofLogNotice("ofxTrueTypeFontUC") << "NEED TO MIX WITH LAST";
                ofLogNotice("ofxTrueTypeFontUC") << "last is " << lastPoint.x << " " << lastPoint.y;
              }
            }
          }
          
          int nextIndex = j+1;
          if (nextIndex >= endPos) {
            nextIndex = startPos;
          }
          
          ofPoint nextPoint((float)vec[nextIndex].x, -(float)vec[nextIndex].y);
          
          if (printVectorInfo) {
            ofLogNotice("ofxTrueTypeFontUC") << "--- last point is " << lastPoint.x << " " <<  lastPoint.y;
          }
          
          bool nextIsConnic = (FT_CURVE_TAG( tags[nextIndex]) != FT_CURVE_TAG_ON) && (FT_CURVE_TAG( tags[nextIndex]) != FT_CURVE_TAG_CUBIC);
          
          //create a 'virtual on point' if we have two connic points
          if(nextIsConnic) {
            nextPoint = (conicPoint + nextPoint) / 2;
            if (printVectorInfo) {
              ofLogNotice("ofxTrueTypeFontUC") << "|_______ double connic!";
            }
          }
          if (printVectorInfo) {
            ofLogNotice("ofxTrueTypeFontUC") << "--- next point is " << nextPoint.x << " " << nextPoint.y;
          }
          
          //quad_bezier(testOutline, lastPoint.x, lastPoint.y, conicPoint.x, conicPoint.y, nextPoint.x, nextPoint.y, 8);
          charOutlines.quadBezierTo(lastPoint.x/64, lastPoint.y/64, conicPoint.x/64, conicPoint.y/64, nextPoint.x/64, nextPoint.y/64);
          
          if (nextIsConnic) {
            lastPoint = nextPoint;
          }
        }
      }
      
      //end for
    }
    charOutlines.close();
  }
  
  return charOutlines;
}

#if defined(TARGET_ANDROID) || defined(TARGET_OF_IOS)
#include <set>
static set<ofxTrueTypeFontUC*> & all_fonts(){
  static set<ofxTrueTypeFontUC*> *all_fonts = new set<ofxTrueTypeFontUC*>;
  return *all_fonts;
}

void ofUnloadAllFontTextures(){
  set<ofxTrueTypeFontUC*>::iterator it;
  for (it=all_fonts().begin(); it!=all_fonts().end(); ++it) {
    (*it)->unloadTextures();
  }
}

void ofReloadAllFontTextures(){
  set<ofxTrueTypeFontUC*>::iterator it;
  for (it=all_fonts().begin(); it!=all_fonts().end(); ++it){
    (*it)->reloadTextures();
  }
}
#endif

#ifdef TARGET_OSX
static string osxFontPathByName( string fontname ){
  CFStringRef targetName = CFStringCreateWithCString(NULL, fontname.c_str(), kCFStringEncodingUTF8);
  CTFontDescriptorRef targetDescriptor = CTFontDescriptorCreateWithNameAndSize(targetName, 0.0);
  CFURLRef targetURL = (CFURLRef) CTFontDescriptorCopyAttribute(targetDescriptor, kCTFontURLAttribute);
  string fontPath = "";
  
  if (targetURL) {
    UInt8 buffer[PATH_MAX];
    CFURLGetFileSystemRepresentation(targetURL, true, buffer, PATH_MAX);
    fontPath = string((char *)buffer);
    CFRelease(targetURL);
  }
  
  CFRelease(targetName);
  CFRelease(targetDescriptor);
  
  return fontPath;
}
#endif

#ifdef TARGET_WIN32
#include <map>
// font font face -> file name name mapping
static map<string, string> fonts_table;
// read font linking information from registry, and store in std::map
void initWindows(){
  LONG l_ret;
  
  const wchar_t *Fonts = L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Fonts";
  
  HKEY key_ft;
  l_ret = RegOpenKeyExW(HKEY_LOCAL_MACHINE, Fonts, 0, KEY_QUERY_VALUE, &key_ft);
  if (l_ret != ERROR_SUCCESS) {
    ofLogError("ofxTrueTypeFontUC") << "initWindows(): couldn't find fonts registery key";
    return;
  }
  
  DWORD value_count;
  DWORD max_data_len;
  wchar_t value_name[2048];
  BYTE *value_data;
  
  
  // get font_file_name -> font_face mapping from the "Fonts" registry key
  
  l_ret = RegQueryInfoKeyW(key_ft, NULL, NULL, NULL, NULL, NULL, NULL, &value_count, NULL, &max_data_len, NULL, NULL);
  if (l_ret != ERROR_SUCCESS) {
    ofLogError("ofxTrueTypeFontUC") << "initWindows(): couldn't query registery for fonts";
    return;
  }
  
  // no font installed
  if (value_count == 0) {
    ofLogError("ofxTrueTypeFontUC") << "initWindows(): couldn't find any fonts in registery";
    return;
  }
  
  // max_data_len is in BYTE
  value_data = static_cast<BYTE *>(HeapAlloc(GetProcessHeap(), HEAP_GENERATE_EXCEPTIONS, max_data_len));
  if (value_data == NULL) return;
  
  char value_name_char[2048];
  char value_data_char[2048];
  /*char ppidl[2048];
   char fontsPath[2048];
   SHGetKnownFolderIDList(FOLDERID_Fonts, 0, NULL, &ppidl);
   SHGetPathFromIDList(ppidl,&fontsPath);*/
  string fontsDir = getenv ("windir");
  fontsDir += "\\Fonts\\";
  for (DWORD i = 0; i < value_count; ++i) {
    DWORD name_len = 2048;
    DWORD data_len = max_data_len;
    
    l_ret = RegEnumValueW(key_ft, i, value_name, &name_len, NULL, NULL, value_data, &data_len);
    if (l_ret != ERROR_SUCCESS) {
      ofLogError("ofxTrueTypeFontUC") << "initWindows(): couldn't read registry key for font type";
      continue;
    }
    
    wcstombs(value_name_char,value_name,2048);
    wcstombs(value_data_char,reinterpret_cast<wchar_t *>(value_data),2048);
    string curr_face = value_name_char;
    string font_file = value_data_char;
    curr_face = curr_face.substr(0, curr_face.find('(') - 1);
    fonts_table[curr_face] = fontsDir + font_file;
  }
  
  HeapFree(GetProcessHeap(), 0, value_data);
  
  l_ret = RegCloseKey(key_ft);
}


static string winFontPathByName( string fontname ){
  if (fonts_table.find(fontname)!=fonts_table.end()) {
    return fonts_table[fontname];
  }
  for (map<string,string>::iterator it = fonts_table.begin(); it!=fonts_table.end(); it++) {
    if (ofIsStringInString(ofToLower(it->first),ofToLower(fontname)))
      return it->second;
  }
  return "";
}
#endif

#ifdef TARGET_LINUX
static string linuxFontPathByName(string fontname){
  string filename;
  FcPattern * pattern = FcNameParse((const FcChar8*)fontname.c_str());
  FcBool ret = FcConfigSubstitute(0,pattern,FcMatchPattern);
  if (!ret) {
    ofLogError() << "linuxFontPathByName(): couldn't find font file or system font with name \"" << fontname << "\"";
    return "";
  }
  FcDefaultSubstitute(pattern);
  FcResult result;
  FcPattern * fontMatch=NULL;
  fontMatch = FcFontMatch(0,pattern,&result);
  
  if (!fontMatch) {
    ofLogError() << "linuxFontPathByName(): couldn't match font file or system font with name \"" << fontname << "\"";
    return "";
  }
  FcChar8 *file;
  if (FcPatternGetString (fontMatch, FC_FILE, 0, &file) == FcResultMatch) {
    filename = (const char*)file;
  }
  else {
    ofLogError() << "linuxFontPathByName(): couldn't find font match for \"" << fontname << "\"";
    return "";
  }
  return filename;
}
#endif

bool ofxTrueTypeFontUC::Impl::initLibraries(){
  if (!librariesInitialized) {
    FT_Error err;
    err = FT_Init_FreeType( &library );
    
    if (err) {
      ofLogError("ofxTrueTypeFontUC") << "loadFont(): couldn't initialize Freetype lib: FT_Error " << err;
      return false;
    }
#ifdef TARGET_LINUX
    FcBool result = FcInit();
    if (!result) {
      return false;
    }
#endif
#ifdef TARGET_WIN32
    initWindows();
#endif
    librariesInitialized = true;
  }
  return true;
}

void ofxTrueTypeFontUC::Impl::finishLibraries(){
  if (librariesInitialized) {
#ifdef TARGET_LINUX
    //FcFini();
#endif
    FT_Done_FreeType(library);
  }
}


//------------------------------------------------------------------
ofxTrueTypeFontUC::ofxTrueTypeFontUC(){
  mImpl = new Impl();
  mImpl->bLoadedOk = false;
  mImpl->bMakeContours = false;
  
#if defined(TARGET_ANDROID) || defined(TARGET_OF_IOS)
  mImpl->all_fonts().insert(this);
#endif
  mImpl->letterSpacing = 1;
  mImpl->spaceSize = 1;
  
  // 3 pixel border around the glyph
  // We show 2 pixels of this, so that blending looks good.
  // 1 pixels is hidden because we don't want to see the real edge of the texture
  
  mImpl->border = 3;
  
  mImpl->stringQuads.setMode(OF_PRIMITIVE_TRIANGLES);
  mImpl->binded = false;
}

//------------------------------------------------------------------
ofxTrueTypeFontUC::~ofxTrueTypeFontUC(){
  
  if (mImpl->bLoadedOk) {
    mImpl->unloadTextures();
  }
  
#if defined(TARGET_ANDROID) || defined(TARGET_OF_IOS)
  mImpl->all_fonts().erase(this);
#endif
  
  if (mImpl != NULL)
    delete mImpl;
}

void ofxTrueTypeFontUC::Impl::unloadTextures(){
  if(!bLoadedOk) return;
  
  texAtlas.clear();
  charOutlines.clear();
  charOutlinesNonVFlipped.clear();
  loadedChars.clear();
  cps.clear();
  
  bLoadedOk = false;
}

void ofxTrueTypeFontUC::reloadFont(){
  loadFont(mImpl->filename, mImpl->fontSize, mImpl->bAntiAliased, mImpl->bMakeContours, mImpl->simplifyAmt, mImpl->dpi);
}

static bool loadFontFace(string fontname, int _fontSize, FT_Face & face, string & filename){
  filename = ofToDataPath(fontname,true);
  ofFile fontFile(filename, ofFile::Reference);
  int fontID = 0;
  if (!fontFile.exists()) {
#ifdef TARGET_LINUX
    filename = linuxFontPathByName(fontname);
#elif defined(TARGET_OSX)
    if (fontname==OF_TTFUC_SANS) {
      fontname = "Helvetica Neue";
      fontID = 4;
    }
    else if (fontname==OF_TTFUC_SERIF) {
      fontname = "Times New Roman";
    }
    else if (fontname==OF_TTFUC_MONO) {
      fontname = "Menlo Regular";
    }
    filename = osxFontPathByName(fontname);
#elif defined(TARGET_WIN32)
    if (fontname==OF_TTFUC_SANS) {
      fontname = "Arial";
    }
    else if (fontname==OF_TTFUC_SERIF) {
      fontname = "Times New Roman";
    }
    else if (fontname==OF_TTFUC_MONO) {
      fontname = "Courier New";
    }
    filename = winFontPathByName(fontname);
#endif
    if (filename == "" ) {
      ofLogError("ofxTrueTypeFontUC") << "loadFontFace(): couldn't find font \"" << fontname << "\"";
      return false;
    }
    ofLogVerbose("ofxTrueTypeFontUC") << "loadFontFace(): \"" << fontname << "\" not a file in data loading system font from \"" << filename << "\"";
  }
  FT_Error err;
  err = FT_New_Face( library, filename.c_str(), fontID, &face );
  if (err) {
    // simple error table in lieu of full table (see fterrors.h)
    string errorString = "unknown freetype";
    if(err == 1) errorString = "INVALID FILENAME";
    ofLogError("ofxTrueTypeFontUC") << "loadFontFace(): couldn't create new face for \"" << fontname << "\": FT_Error " << err << " " << errorString;
    return false;
  }
  
  return true;
}

//-----------------------------------------------------------
bool ofxTrueTypeFontUC::loadFont(const string &_filename, int _fontSize, bool _bAntiAliased, bool _makeContours, float _simplifyAmt, int _dpi){
  mImpl->initLibraries();
  
  //------------------------------------------------
  if (mImpl->bLoadedOk == true) {
    // we've already been loaded, try to clean up :
    mImpl->unloadTextures();
  }
  //------------------------------------------------
  
  if ( _dpi == 0 ) {
    _dpi = ttfGlobalDpi;
  }
  
  mImpl->bLoadedOk = false;
  mImpl->bAntiAliased = _bAntiAliased;
  mImpl->fontSize = _fontSize;
  mImpl->bMakeContours = _makeContours;
  mImpl->simplifyAmt = _simplifyAmt;
  mImpl->dpi = _dpi;
  
  mImpl->encoding = OF_ENCODING_UTF8;
  
  //--------------- load the library and typeface
  if (!loadFontFace(_filename, _fontSize, face, mImpl->filename)) {
    return false;
  }
  
  
  FT_Set_Char_Size( face, mImpl->fontSize << 6, mImpl->fontSize << 6, mImpl->dpi, mImpl->dpi);
  mImpl->lineHeight = mImpl->fontSize * 1.43f;
  
  //------------------------------------------------------
  //kerning would be great to support:
  //ofLogNotice("ofxTrueTypeFontUC") << "FT_HAS_KERNING ? " <<  FT_HAS_KERNING(face);
  //------------------------------------------------------
  
  if (mImpl->bMakeContours) {
    mImpl->charOutlines.clear();
  }
  
  // ------------- get default space size
  unsigned int charID = mImpl->getCharID('p');
  mImpl->loadChar(charID);
  
  // ------------- close the library and typeface
  mImpl->bLoadedOk = true;
  return true;
}

bool ofxTrueTypeFontUC::Impl::loadChar(const unsigned int &charID){
  //------------------------------------------ anti aliased or not:
  FT_Error err = FT_Load_Glyph( face, FT_Get_Char_Index( face, loadedChars[charID] ), FT_LOAD_DEFAULT );
  if (err) {
    ofLogError("ofxTrueTypeFontUC") << "loadChar(): FT_Load_Glyph failed for code point " << loadedChars[charID] << ": FT_Error " << err;
    return false;
  }
  
  if (bAntiAliased == true)
    FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL);
  else
    FT_Render_Glyph(face->glyph, FT_RENDER_MODE_MONO);
  
  //------------------------------------------
  FT_Bitmap& bitmap= face->glyph->bitmap;
  
  
  // prepare the texture:
  /*int width  = ofNextPow2( bitmap.width + border*2 );
   int height = ofNextPow2( bitmap.rows  + border*2 );
   
   
   // ------------------------- this is fixing a bug with small type
   // ------------------------- appearantly, opengl has trouble with
   // ------------------------- width or height textures of 1, so we
   // ------------------------- we just set it to 2...
   if (width == 1) width = 2;
   if (height == 1) height = 2;*/
  
  
  if (bMakeContours) {
    if (printVectorInfo) {
      ofLogNotice("ofxTrueTypeFontUC") <<  "code point " << loadedChars[charID];
    }
    
    charOutlines[charID] = makeContoursForCharacter( face );
    
    charOutlinesNonVFlipped[charID] = charOutlines[charID];
    charOutlinesNonVFlipped[charID].translate(ofVec3f(0,cps[charID].height));
    charOutlinesNonVFlipped[charID].scale(1,-1);
    
    if (simplifyAmt>0)
      charOutlines[charID].simplify(simplifyAmt);
    charOutlines[charID].getTessellation();
    
    if (simplifyAmt>0)
      charOutlinesNonVFlipped[charID].simplify(simplifyAmt);
    charOutlinesNonVFlipped[charID].getTessellation();
  }
  
  
  // -------------------------
  // info about the character:
  cps[charID].character = loadedChars[charID];
  cps[charID].height = face->glyph->bitmap_top;
  cps[charID].width = face->glyph->bitmap.width;
  cps[charID].setWidth = face->glyph->advance.x >> 6;
  cps[charID].topExtent = face->glyph->bitmap.rows;
  cps[charID].leftExtent = face->glyph->bitmap_left;
  
  int width = cps[charID].width;
  int height = bitmap.rows;
  
  
  cps[charID].tW = width;
  cps[charID].tH = height;
  
  GLint fheight = cps[charID].height;
  GLint bwidth = cps[charID].width;
  GLint top = cps[charID].topExtent - cps[charID].height;
  GLint lextent = cps[charID].leftExtent;
  
  GLfloat corr, stretch;
  
  //this accounts for the fact that we are showing 2*visibleBorder extra pixels
  //so we make the size of each char that many pixels bigger
  stretch = 0;//(float)(visibleBorder * 2);
  
  corr = (float)(( (fontSize - fheight) + top) - fontSize);
  
  cps[charID].x1 = lextent + bwidth + stretch;
  cps[charID].y1 = fheight + corr + stretch;
  cps[charID].x2 = (float) lextent;
  cps[charID].y2 = -top + corr;
  
  
  // Allocate Memory For The Texture Data.
  ofPixels expanded_data;
  expanded_data.allocate(width, height, 2);
  //-------------------------------- clear data:
  expanded_data.set(0,255); // every luminance pixel = 255
  expanded_data.set(1,0);
  
  
  if (bAntiAliased == true) {
    ofPixels bitmapPixels;
    bitmapPixels.setFromExternalPixels(bitmap.buffer, bitmap.width, bitmap.rows, 1);
    expanded_data.setChannel(1, bitmapPixels);
  }
  else {
    //-----------------------------------
    // true type packs monochrome info in a
    // 1-bit format, hella funky
    // here we unpack it:
    unsigned char *src =  bitmap.buffer;
    for (int j=0; j <bitmap.rows;j++) {
      unsigned char b = 0;
      unsigned char *bptr = src;
      for (int k=0; k < bitmap.width; k++) {
        expanded_data[2*(k+j*width)] = 255;
        
        if (k%8==0) {
          b = (*bptr++);
        }
        
        expanded_data[2*(k+j*width) + 1] = b&0x80 ? 255 : 0;
        b <<= 1;
      }
      src += bitmap.pitch;
    }
    //-----------------------------------
  }
  
  
  // pack in a texture, algorithm to calculate min w/h from
  // http://upcommons.upc.edu/pfc/bitstream/2099.1/7720/1/TesiMasterJonas.pdf
  //ofLogNotice("ofTrueTypeFont") << "loadFont(): areaSum: " << areaSum
  float alpha = logf((cps[charID].width+border*2) * (cps[charID].height+border*2)) * 1.44269;
  
  int w = width + 2 * border;
  int h = height + 2 * border;
  
  ofPixels atlasPixelsLuminanceAlpha;
  atlasPixelsLuminanceAlpha.allocate(w, h, 2);
  atlasPixelsLuminanceAlpha.set(0, 255);
  atlasPixelsLuminanceAlpha.set(1, 0);
  
  
  int x = 0;
  int y = 0;
  int maxRowHeight = cps[charID].tH + border * 2;
  ofPixels & charPixels = expanded_data;
  
  if (x+cps[charID].tW + border*2>w) {
    x = 0;
    y += maxRowHeight;
    maxRowHeight = cps[charID].tH + border * 2;
  }
  
  cps[charID].t2 = float(x + border) / float(w);
  cps[charID].v2 = float(y + border) / float(h);
  cps[charID].t1 = float(cps[charID].tW + x + border) / float(w);
  cps[charID].v1 = float(cps[charID].tH + y + border) / float(h);
  charPixels.pasteInto(atlasPixelsLuminanceAlpha,x+border,y+border);
  
  ofPixels atlasPixels;
  atlasPixels.allocate(atlasPixelsLuminanceAlpha.getWidth(),atlasPixelsLuminanceAlpha.getHeight(),4);
  atlasPixels.setChannel(0,atlasPixelsLuminanceAlpha.getChannel(0));
  atlasPixels.setChannel(1,atlasPixelsLuminanceAlpha.getChannel(0));
  atlasPixels.setChannel(2,atlasPixelsLuminanceAlpha.getChannel(0));
  atlasPixels.setChannel(3,atlasPixelsLuminanceAlpha.getChannel(1));
  texAtlas[charID].allocate(atlasPixels,false);
  
  if (bAntiAliased && fontSize>20) {
    texAtlas[charID].setTextureMinMagFilter(GL_LINEAR,GL_LINEAR);
  }
  else {
    texAtlas[charID].setTextureMinMagFilter(GL_NEAREST,GL_NEAREST);
  }
  texAtlas[charID].loadData(atlasPixels);
  
  return true;
}

ofTextEncoding ofxTrueTypeFontUC::getEncoding() const {
  return mImpl->encoding;
}

//-----------------------------------------------------------
bool ofxTrueTypeFontUC::isLoaded() {
  return mImpl->bLoadedOk;
}

//-----------------------------------------------------------
bool ofxTrueTypeFontUC::isAntiAliased() {
  return mImpl->bAntiAliased;
}

//-----------------------------------------------------------
int ofxTrueTypeFontUC::getSize() {
  return mImpl->fontSize;
}

//-----------------------------------------------------------
void ofxTrueTypeFontUC::setLineHeight(float _newLineHeight) {
  mImpl->lineHeight = _newLineHeight;
}

//-----------------------------------------------------------
float ofxTrueTypeFontUC::getLineHeight(){
  return mImpl->lineHeight;
}

//-----------------------------------------------------------
void ofxTrueTypeFontUC::setLetterSpacing(float _newletterSpacing) {
  mImpl->letterSpacing = _newletterSpacing;
}

//-----------------------------------------------------------
float ofxTrueTypeFontUC::getLetterSpacing(){
  return mImpl->letterSpacing;
}

//-----------------------------------------------------------
void ofxTrueTypeFontUC::setSpaceSize(float _newspaceSize) {
  mImpl->spaceSize = _newspaceSize;
}

//-----------------------------------------------------------
float ofxTrueTypeFontUC::getSpaceSize(){
  return mImpl->spaceSize;
}

//------------------------------------------------------------------
unsigned int ofxTrueTypeFontUC::Impl::getCharID(const unsigned int &utf32_codepoint){
  if (loadedChars.size() >= kLimitCharactersNum) {
    ofLog(OF_LOG_ERROR,"ofxTrueTypeFontUC::getCharID - Error : too many typeface already loaded - call reloadFont to reset");
    return 0;
  }
  
  for (unsigned int i = 0; i < loadedChars.size(); ++i) {
    if (loadedChars[i] == utf32_codepoint) {
      return i;
    }
  }
  
  if (bMakeContours) {
    charOutlines.push_back(ofPath());
    charOutlinesNonVFlipped.push_back(ofPath());
  }
  
  loadedChars.push_back(utf32_codepoint);
  cps.push_back(charPropsUC());
  texAtlas.push_back(ofTexture());
  
  return loadedChars.size() - 1;
}

//------------------------------------------------------------------
ofPath ofxTrueTypeFontUC::Impl::getCharacterAsPoints(const unsigned int &charID, bool vflip){
  if ( bMakeContours == false ) {
    ofLogError("ofxTrueTypeFontUC") << "getCharacterAsPoints(): contours not created, call loadFont() with makeContours set to true";
    return ofPath();
  }
  
  if (vflip) {
    return charOutlines[charID];
  }
  else {
    return charOutlinesNonVFlipped[charID];
  }
}

//-----------------------------------------------------------
void ofxTrueTypeFontUC::Impl::drawChar(const unsigned int &charID, float x, float y){
  GLfloat x1, y1, x2, y2;
  GLfloat t1, v1, t2, v2;
  t2 = cps[charID].t2;
  v2 = cps[charID].v2;
  t1 = cps[charID].t1;
  v1 = cps[charID].v1;
  
  x1 = cps[charID].x1+x;
  y1 = cps[charID].y1;
  x2 = cps[charID].x2+x;
  y2 = cps[charID].y2;
  
  int firstIndex = stringQuads.getVertices().size();
  
  if (!ofIsVFlipped()) {
    y1 *= -1;
    y2 *= -1;
  }
  
  y1 += y;
  y2 += y;
  
  stringQuads.addVertex(ofVec3f(x1, y1));
  stringQuads.addVertex(ofVec3f(x2, y1));
  stringQuads.addVertex(ofVec3f(x2, y2));
  stringQuads.addVertex(ofVec3f(x1, y2));
  
  stringQuads.addTexCoord(ofVec2f(t1, v1));
  stringQuads.addTexCoord(ofVec2f(t2, v1));
  stringQuads.addTexCoord(ofVec2f(t2, v2));
  stringQuads.addTexCoord(ofVec2f(t1, v2));
  
  stringQuads.addIndex(firstIndex);
  stringQuads.addIndex(firstIndex + 1);
  stringQuads.addIndex(firstIndex + 2);
  stringQuads.addIndex(firstIndex + 2);
  stringQuads.addIndex(firstIndex + 3);
  stringQuads.addIndex(firstIndex);
}

//-----------------------------------------------------------
vector<ofPath> ofxTrueTypeFontUC::getStringAsPoints(const string &utf8_src, bool vflip){
  vector<ofPath> shapes;
  
  if (!mImpl->bLoadedOk) {
    ofLogError("ofxTrueTypeFontUC") << "getStringAsPoints(): font not allocated: line " << __LINE__ << " in " << __FILE__;
    return shapes;
  };
  
  GLint index = 0;
  GLfloat X = 0;
  GLfloat Y = 0;
  int newLineDirection = 1;
  
  if (vflip) {
    // this would align multiline texts to the last line when vflip is disabled
    //int lines = ofStringTimesInString(c,"\n");
    //Y = lines*lineHeight;
    newLineDirection = -1;
  }
  
  basic_string<unsigned int> utf32_src = util::ofxTrueTypeFontUC::convUTF8ToUTF32(utf8_src);
  int len = (int)utf32_src.length();
  
  while (index < len) {
    unsigned int charID = mImpl->getCharID(utf32_src[index]);
    if (mImpl->cps[charID].character == mImpl->kTypeFaceUnloaded) {
      mImpl->loadChar(charID);
    }
    if (utf32_src[index] == (unsigned int)'\n') {
      Y += mImpl->lineHeight * newLineDirection;
    }
    else if (utf32_src[index] == (unsigned int)' ') {
      int pID = mImpl->getCharID((unsigned int)'p');
      X += mImpl->cps[charID].setWidth * mImpl->letterSpacing * mImpl->spaceSize;
    }
    else {
      shapes.push_back(mImpl->getCharacterAsPoints(charID, vflip));
      shapes.back().translate(ofPoint(X, Y));
      X += mImpl->cps[charID].setWidth * mImpl->letterSpacing;
    }
    index++;
  }
  
  return shapes;
}

//-----------------------------------------------------------
void ofxTrueTypeFontUC::Impl::drawCharAsShape(const unsigned int &charID, float x, float y) {
  if (ofIsVFlipped()) {
    ofPath & charRef = charOutlines[charID];
    charRef.setFilled(ofGetStyle().bFill);
    charRef.draw(x,y);
  }
  else {
    ofPath & charRef = charOutlinesNonVFlipped[charID];
    charRef.setFilled(ofGetStyle().bFill);
    charRef.draw(x,y);
  }
}

//-----------------------------------------------------------
float ofxTrueTypeFontUC::stringWidth(const string &c) {
  ofRectangle rect = getStringBoundingBox(c, 0,0);
  return rect.width;
}


ofRectangle ofxTrueTypeFontUC::getStringBoundingBox(const string &utf8_src, float x, float y){
  ofRectangle myRect;
  
  if (!mImpl->bLoadedOk) {
    ofLogError("ofxTrueTypeFontUC") << "getStringBoundingBox(): font not allocated";
    return myRect;
  }
  
  basic_string<unsigned int> utf32_src = util::ofxTrueTypeFontUC::convUTF8ToUTF32(utf8_src);
  int len = (int)utf32_src.length();
  
  GLint index = 0;
  GLfloat xoffset = 0;
  GLfloat yoffset = 0;
  float minx = -1;
  float miny = -1;
  float maxx = -1;
  float maxy = -1;
  
  if (len < 1 || mImpl->cps.empty()) {
    myRect.x = 0;
    myRect.y = 0;
    myRect.width = 0;
    myRect.height = 0;
    return myRect;
  }
  
  bool bFirstCharacter = true;
  while (index < len) {
    unsigned int charID = mImpl->getCharID(utf32_src[index]);
    if (mImpl->cps[charID].character == mImpl->kTypeFaceUnloaded) {
      mImpl->loadChar(charID);
    }
    if (utf32_src[index] == (unsigned int)'\n') {
      yoffset += mImpl->lineHeight;
      xoffset = 0 ; //reset X Pos back to zero
    }
    else if (utf32_src[index] == (unsigned int)' ') {
      int pID = mImpl->getCharID((unsigned int)'p');
      xoffset += mImpl->cps[pID].setWidth * mImpl->letterSpacing * mImpl->spaceSize;
      // zach - this is a bug to fix -- for now, we don't currently deal with ' ' in calculating string bounding box
    }
    else {
      GLint height = mImpl->cps[charID].height;
      GLint bwidth = mImpl->cps[charID].width * mImpl->letterSpacing;
      GLint top = mImpl->cps[charID].topExtent - mImpl->cps[charID].height;
      GLint lextent = mImpl->cps[charID].leftExtent;
      float x1, y1, x2, y2, corr, stretch;
      stretch = 0;//(float)visibleBorder * 2;
      corr = (float)(((mImpl->fontSize - height) + top) - mImpl->fontSize);
      x1 = (x + xoffset + lextent + bwidth + stretch);
      y1 = (y + yoffset + height + corr + stretch);
      x2 = (x + xoffset + lextent);
      y2 = (y + yoffset + -top + corr);
      xoffset += mImpl->cps[charID].setWidth * mImpl->letterSpacing;
      if (bFirstCharacter == true) {
        minx = x2;
        miny = y2;
        maxx = x1;
        maxy = y1;
        bFirstCharacter = false;
      }
      else {
        if (x2 < minx) minx = x2;
        if (y2 < miny) miny = y2;
        if (x1 > maxx) maxx = x1;
        if (y1 > maxy) maxy = y1;
      }
    }
    index++;
  }
  
  myRect.x = minx;
  myRect.y = miny;
  myRect.width = maxx-minx;
  myRect.height = maxy-miny;
  return myRect;
}

//-----------------------------------------------------------
float ofxTrueTypeFontUC::stringHeight(const string &c){
  ofRectangle rect = getStringBoundingBox(c, 0,0);
  return rect.height;
}

//=====================================================================
void ofxTrueTypeFontUC::drawString(const string &utf8_src, float x, float y){
  if (!mImpl->bLoadedOk) {
    ofLog(OF_LOG_ERROR,"ofxTrueTypeFontUC::drawString - Error : font not allocated -- line %d in %s", __LINE__,__FILE__);
    return;
  };
  
  GLint index = 0;
  GLfloat X = x;
  GLfloat Y = y;
  int newLineDirection = 1;
  
  if (!ofIsVFlipped()) {
    // this would align multiline texts to the last line when vflip is disabled
    //int lines = ofStringTimesInString(c,"\n");
    //Y = lines*lineHeight;
    newLineDirection = -1;
  }
  
  basic_string<unsigned int> utf32_src = util::ofxTrueTypeFontUC::convUTF8ToUTF32(utf8_src);
  int len = (int)utf32_src.length();
  
  while (index < len) {
    unsigned int charID = mImpl->getCharID(utf32_src[index]);
    if (mImpl->cps[charID].character == mImpl->kTypeFaceUnloaded) {
      mImpl->loadChar(charID);
    }
    if (utf32_src[index] == (unsigned int)'\n') {
      Y += mImpl->lineHeight * newLineDirection;
      X = x ; //reset X Pos back to zero
    }
    else if (utf32_src[index] == (unsigned int)' ') {
      int pID = mImpl->getCharID((unsigned int)'p');
      X += mImpl->cps[charID].setWidth * mImpl->letterSpacing * mImpl->spaceSize;
    }
    else {
      mImpl->bind(charID);
      mImpl->drawChar(charID, X, Y);
      mImpl->unbind(charID);
      X += mImpl->cps[charID].setWidth * mImpl->letterSpacing;
    }
    
    index++;
  }
}

//-----------------------------------------------------------
void ofxTrueTypeFontUC::Impl::bind(const unsigned int &charID){
  // we need transparency to draw text, but we don't know
  // if that is set up in outside of this function
  // we "pushAttrib", turn on alpha and "popAttrib"
  // http://www.opengl.org/documentation/specs/man_pages/hardcopy/GL/html/gl/pushattrib.html
  
  // **** note ****
  // I have read that pushAttrib() is slow, if used often,
  // maybe there is a faster way to do this?
  // ie, check if blending is enabled, etc...
  // glIsEnabled().... glGet()...
  // http://www.opengl.org/documentation/specs/man_pages/hardcopy/GL/html/gl/get.html
  // **************
  // (a) record the current "alpha state, blend func, etc"
  
  blend_enabled = glIsEnabled(GL_BLEND);
  texture_2d_enabled = glIsEnabled(GL_TEXTURE_2D);
  glGetIntegerv( GL_BLEND_SRC, &blend_src );
  glGetIntegerv( GL_BLEND_DST, &blend_dst );
  
  // (b) enable our regular ALPHA blending!
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  
  texAtlas[charID].bind();
  stringQuads.clear();
}

//-----------------------------------------------------------
void ofxTrueTypeFontUC::Impl::unbind(const unsigned int &charID){
  stringQuads.drawFaces();
  texAtlas[charID].unbind();
  
  if( !blend_enabled )
    glDisable(GL_BLEND);
  if( !texture_2d_enabled )
    glDisable(GL_TEXTURE_2D);
  glBlendFunc( blend_src, blend_dst );
}

//=====================================================================
void ofxTrueTypeFontUC::drawStringAsShapes(const string &utf8_src, float x, float y){
  
  if (!mImpl->bLoadedOk) {
    ofLogError("ofxTrueTypeFontUC") << "drawStringAsShapes(): font not allocated: line " << __LINE__ << " in " << __FILE__;
    return;
  };
  
  //----------------------- error checking
  if (!mImpl->bMakeContours) {
    ofLogError("ofxTrueTypeFontUC") << "drawStringAsShapes(): contours not created for this font, call loadFont() with makeContours set to true";
    return;
  }
  
  GLint index = 0;
  GLfloat X = x;
  GLfloat Y = y;
  int newLineDirection = 1;
  
  if (!ofIsVFlipped()) {
    // this would align multiline texts to the last line when vflip is disabled
    //int lines = ofStringTimesInString(c,"\n");
    //Y = lines*lineHeight;
    newLineDirection = -1;
  }
  
  basic_string<unsigned int> utf32_src = util::ofxTrueTypeFontUC::convUTF8ToUTF32(utf8_src);
  int len = (int)utf32_src.length();
  
  while (index < len) {
    unsigned int charID = mImpl->getCharID(utf32_src[index]);
    if (mImpl->cps[charID].character == mImpl->kTypeFaceUnloaded) {
      mImpl->loadChar(charID);
    }
    
    if (utf32_src[index] == (unsigned int)'\n') {
      Y += mImpl->lineHeight * newLineDirection;
      X = x ; //reset X Pos back to zero
    }
    else if (utf32_src[index] == (unsigned int)' ') {
      int pID = mImpl->getCharID((unsigned int)'p');
      X += mImpl->cps[charID].setWidth * mImpl->letterSpacing * mImpl->spaceSize;
    }
    else {
      mImpl->drawCharAsShape(charID, X, Y);
      X += mImpl->cps[charID].setWidth * mImpl->letterSpacing;
    }
    
    index++;
  }
}

//-----------------------------------------------------------
int ofxTrueTypeFontUC::getNumCharacters() {
  return mImpl->loadedChars.size();
}

//===========================================================
namespace util {
  namespace ofxTrueTypeFontUC {
    
basic_string<unsigned int> convUTF8ToUTF32(const std::string &utf8_src) {
  basic_string<unsigned int> dst;
  
  // convert UTF-8 on char or wchar_t to UCS-4 on wchar_t
  int size = utf8_src.size();
  int index = 0;
  while (index < size) {
    wchar_t c = (unsigned char)utf8_src[index];
    if (c < 0x80) {
      dst += (c);
    }
    else if (c < 0xe0) {
      if (index + 1 < size) {
        dst += (((c & 0x1f) << 6) | (utf8_src[index+1] & 0x3f));
        index++;
      }
    }
    else if (c < 0xf0) {
      if (index + 2 < size) {
        dst += (((c & 0x0f) << 12) | ((utf8_src[index+1] & 0x3f) << 6) |
                (utf8_src[index+2] & 0x3f));
        index += 2;
      }
    }
    else if (c < 0xf8) {
      if (index + 3 < size) {
        dst += (((c & 0x07) << 18) | ((utf8_src[index+1] & 0x3f) << 12) |
                ((utf8_src[index+2] & 0x3f) << 6) | (utf8_src[index+3] & 0x3f));
        index += 3;
      }
    }
    else if (c < 0xfc) {
      if (index + 4 < size) {
        dst += (((c & 0x03) << 24) | ((utf8_src[index+1] & 0x3f) << 18) |
                ((utf8_src[index+2] & 0x3f) << 12) | ((utf8_src[index+3] & 0x3f) << 6) |
                (utf8_src[index+4] & 0x3f));
        index += 4;
      }
    }
    else if (c < 0xfe) {
      if (index + 5 < size) {
        dst += (((c & 0x01) << 30) | ((utf8_src[index+1] & 0x3f) << 24) |
                ((utf8_src[index+2] & 0x3f) << 18) | ((utf8_src[index+3] & 0x3f) << 12) |
                ((utf8_src[index+4] & 0x3f) << 6) | (utf8_src[index+5] & 0x3f));
        index += 5;
      }
    }
    index++;
  }
  
  return dst;
}
    
  } // namespace ofxTrueTypeFontUC
} // namespace util

