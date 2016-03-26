#include "ofxTrueTypeFontUC.h"
//--------------------------

#include "ft2build.h"

#ifdef TARGET_LINUX
#include FT_FREETYPE_H
#include FT_GLYPH_H
#include FT_OUTLINE_H
#include FT_TRIGONOMETRY_H
#include <fontconfig/fontconfig.h>
#else
#if (OF_VERSION_MAJOR == 0) && (OF_VERSION_MINOR <= 8)
#include "freetype2/freetype/freetype.h"
#include "freetype2/freetype/ftglyph.h"
#include "freetype2/freetype/ftoutln.h"
#include "freetype2/freetype/fttrigon.h"
#else
#include "freetype.h"
#include "ftglyph.h"
#include "ftoutln.h"
#include "fttrigon.h"
#endif
#endif

#include <algorithm>

#ifdef TARGET_WIN32
#include <windows.h>
#include <codecvt>
#include <locale>
#endif

#include "ofPoint.h"
#include "ofConstants.h"
#include "ofTexture.h"
#include "ofMesh.h"
#include "ofUtils.h"
#include "ofGraphics.h"



//===========================================================
#ifdef TARGET_WIN32

static const basic_string<unsigned int> convToUTF32(const string &src) {
  if (src.size() == 0) {
    return basic_string<unsigned int> ();
  }
  
  // convert XXX -> UTF-16
  const int n_size = ::MultiByteToWideChar(CP_ACP, 0, src.c_str(), -1, NULL, 0);
  vector<wchar_t> buffUTF16(n_size);
  ::MultiByteToWideChar(CP_ACP, 0, src.c_str(), -1, &buffUTF16[0], n_size);
  
  // convert UTF-16 -> UTF-8
  const int utf8str_size = ::WideCharToMultiByte(CP_UTF8, 0, &buffUTF16[0], -1, NULL, 0, NULL, 0);
  vector<char> buffUTF8(utf8str_size);
  ::WideCharToMultiByte(CP_UTF8, 0, &buffUTF16[0], -1, &buffUTF8[0], utf8str_size, NULL, 0);
  
  // convert UTF-8 -> UTF-32 (UCS-4)
  std::wstring_convert<std::codecvt_utf8<unsigned int>, unsigned int> convert32;
  return convert32.from_bytes(&buffUTF8[0]);
}

#else

static const basic_string<unsigned int> convToUTF32(const string &utf8_src){
  basic_string<unsigned int> dst;
  
  // convert UTF-8 -> UTF-32 (UCS-4)
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

#endif

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


//---------------------------------------------------
class ofxTrueTypeFontUC::Impl {
public:
  Impl() :librariesInitialized_(false){};
  ~Impl() {};

  bool implLoadFont(string filename, int fontsize, bool _bAntiAliased, bool makeContours, float _simplifyAmt, int dpi);
  void implReserveCharacters(int num);
  void implUnloadFont();

  bool bLoadedOk_;
  bool bAntiAliased_;
  int limitCharactersNum_;
  float simplifyAmt_;
  int dpi_;

  vector <ofPath> charOutlines;

  float lineHeight_;
  float letterSpacing_;
  float	spaceSize_;

  vector<charPropsUC> cps;  // properties for each character

  int	fontSize_;
  bool bMakeContours_;

  void drawChar(int c, float x, float y);
  void drawCharAsShape(int c, float x, float y);

  int	border_;  // visibleBorder;
  string filename_;

  // ofTexture texAtlas;
  vector<ofTexture> textures;
  bool binded_;
  ofMesh stringQuads;

  typedef struct FT_LibraryRec_ * FT_Library;
  typedef struct FT_FaceRec_ * FT_Face;
  FT_Library library_;
  FT_Face face_;
  bool librariesInitialized_;

  bool loadFontFace(string fontname);

  ofPath getCharacterAsPointsFromCharID(const int & charID);

  void bind(const int & charID);
  void unbind(const int & charID);

  int getCharID(const int & c);
  void loadChar(const int & charID);
  vector<int> loadedChars;

  static const int kTypefaceUnloaded;
  static const int kDefaultLimitCharactersNum;

  void unloadTextures();
  bool initLibraries();
  void finishLibraries();

#ifdef TARGET_OPENGLES
  GLint blend_src, blend_dst_;
  GLboolean blend_enabled_;
  GLboolean texture_2d_enabled_;
#endif
};

static bool printVectorInfo_ = false;
static int ttfGlobalDpi_ = 96;

//--------------------------------------------------------
void ofxTrueTypeFontUC::setGlobalDpi(int newDpi){
  ttfGlobalDpi_ = newDpi;
}

//--------------------------------------------------------
static ofPath makeContoursForCharacter(FT_Face & face);

static ofPath makeContoursForCharacter(FT_Face & face) {

  int nContours	= face->glyph->outline.n_contours;
  int startPos	= 0;

  char * tags		= face->glyph->outline.tags;
  FT_Vector * vec = face->glyph->outline.points;

  ofPath charOutlines;
  charOutlines.setUseShapeColor(false);

  for(int k = 0; k < nContours; k++){
    if( k > 0 ){
      startPos = face->glyph->outline.contours[k-1]+1;
    }
    int endPos = face->glyph->outline.contours[k]+1;

    if(printVectorInfo_){
      ofLogNotice("ofxTrueTypeFontUC") << "--NEW CONTOUR";
    }

    //vector <ofPoint> testOutline;
    ofPoint lastPoint;

    for(int j = startPos; j < endPos; j++){

      if( FT_CURVE_TAG(tags[j]) == FT_CURVE_TAG_ON ){
        lastPoint.set((float)vec[j].x, (float)-vec[j].y, 0);
        if(printVectorInfo_){
          ofLogNotice("ofxTrueTypeFontUC") << "flag[" << j << "] is set to 1 - regular point - " << lastPoint.x <<  lastPoint.y;
        }
        if (j==startPos){
          charOutlines.moveTo(lastPoint/64);
        }
        else {
          charOutlines.lineTo(lastPoint/64);
        }

      }else{
        if(printVectorInfo_){
          ofLogNotice("ofxTrueTypeFontUC") << "flag[" << j << "] is set to 0 - control point";
        }

        if( FT_CURVE_TAG(tags[j]) == FT_CURVE_TAG_CUBIC ){
          if(printVectorInfo_){
            ofLogNotice("ofxTrueTypeFontUC") << "- bit 2 is set to 2 - CUBIC";
          }

          int prevPoint = j-1;
          if( j == 0){
            prevPoint = endPos-1;
          }

          int nextIndex = j+1;
          if( nextIndex >= endPos){
            nextIndex = startPos;
          }

          ofPoint nextPoint( (float)vec[nextIndex].x,  -(float)vec[nextIndex].y );

          //we need two control points to draw a cubic bezier
          bool lastPointCubic =  ( FT_CURVE_TAG(tags[prevPoint]) != FT_CURVE_TAG_ON ) && ( FT_CURVE_TAG(tags[prevPoint]) == FT_CURVE_TAG_CUBIC);

          if( lastPointCubic ){
            ofPoint controlPoint1((float)vec[prevPoint].x,	(float)-vec[prevPoint].y);
            ofPoint controlPoint2((float)vec[j].x, (float)-vec[j].y);
            ofPoint nextPoint((float) vec[nextIndex].x,	-(float) vec[nextIndex].y);

            //cubic_bezier(testOutline, lastPoint.x, lastPoint.y, controlPoint1.x, controlPoint1.y, controlPoint2.x, controlPoint2.y, nextPoint.x, nextPoint.y, 8);
            charOutlines.bezierTo(controlPoint1.x/64, controlPoint1.y/64, controlPoint2.x/64, controlPoint2.y/64, nextPoint.x/64, nextPoint.y/64);
          }

        }else{

          ofPoint conicPoint( (float)vec[j].x,  -(float)vec[j].y );

          if(printVectorInfo_){
            ofLogNotice("ofxTrueTypeFontUC") << "- bit 2 is set to 0 - conic- ";
            ofLogNotice("ofxTrueTypeFontUC") << "--- conicPoint point is " << conicPoint.x << conicPoint.y;
          }

          //If the first point is connic and the last point is connic then we need to create a virutal point which acts as a wrap around
          if( j == startPos ){
            bool prevIsConnic = (  FT_CURVE_TAG( tags[endPos-1] ) != FT_CURVE_TAG_ON ) && ( FT_CURVE_TAG( tags[endPos-1]) != FT_CURVE_TAG_CUBIC );

            if( prevIsConnic ){
              ofPoint lastConnic((float)vec[endPos - 1].x, (float)-vec[endPos - 1].y);
              lastPoint = (conicPoint + lastConnic) / 2;

              if(printVectorInfo_){
                ofLogNotice("ofxTrueTypeFontUC") << "NEED TO MIX WITH LAST";
                ofLogNotice("ofxTrueTypeFontUC") << "last is " << lastPoint.x << " " << lastPoint.y;
              }
            }
            else {
              lastPoint.set((float)vec[endPos-1].x, (float)-vec[endPos-1].y, 0);
            }
          }

          //bool doubleConic = false;

          int nextIndex = j+1;
          if( nextIndex >= endPos){
            nextIndex = startPos;
          }

          ofPoint nextPoint( (float)vec[nextIndex].x,  -(float)vec[nextIndex].y );

          if(printVectorInfo_){
            ofLogNotice("ofxTrueTypeFontUC") << "--- last point is " << lastPoint.x << " " <<  lastPoint.y;
          }

          bool nextIsConnic = (  FT_CURVE_TAG( tags[nextIndex] ) != FT_CURVE_TAG_ON ) && ( FT_CURVE_TAG( tags[nextIndex]) != FT_CURVE_TAG_CUBIC );

          //create a 'virtual on point' if we have two connic points
          if( nextIsConnic ){
            nextPoint = (conicPoint + nextPoint) / 2;
            if(printVectorInfo_){
              ofLogNotice("ofxTrueTypeFontUC") << "|_______ double connic!";
            }
          }
          if(printVectorInfo_){
            ofLogNotice("ofxTrueTypeFontUC") << "--- next point is " << nextPoint.x << " " << nextPoint.y;
          }

          //quad_bezier(testOutline, lastPoint.x, lastPoint.y, conicPoint.x, conicPoint.y, nextPoint.x, nextPoint.y, 8);
          charOutlines.quadBezierTo(lastPoint.x/64, lastPoint.y/64, conicPoint.x/64, conicPoint.y/64, nextPoint.x/64, nextPoint.y/64);

          if( nextIsConnic ){
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
    (*it)->unloadFont();
  }
}

void ofReloadAllFontTextures(){
  set<ofxTrueTypeFontUC*>::iterator it;
  for (it=all_fonts().begin(); it!=all_fonts().end(); ++it) {
    (*it)->reloadFont();
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
static void initWindows(){
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

//------------------------------------------------------------------
ofxTrueTypeFontUC::ofxTrueTypeFontUC() {
  mImpl = new Impl();
  mImpl->bLoadedOk_	= false;
  mImpl->bMakeContours_	= false;
  mMaxWidth = 0;
#if defined(TARGET_ANDROID) || defined(TARGET_OF_IOS)
  all_fonts().insert(this);
#endif
  mImpl->letterSpacing_ = 1;
  mImpl->spaceSize_ = 1;

  // 3 pixel border around the glyph
  // We show 2 pixels of this, so that blending looks good.
  // 1 pixels is hidden because we don't want to see the real edge of the texture
  mImpl->border_ = 3;

  mImpl->stringQuads.setMode(OF_PRIMITIVE_TRIANGLES);
  mImpl->binded_ = false;

  mImpl->limitCharactersNum_ = mImpl->kDefaultLimitCharactersNum;
}

//------------------------------------------------------------------
ofxTrueTypeFontUC::~ofxTrueTypeFontUC() {

  if (mImpl->bLoadedOk_)
    unloadFont();

#if defined(TARGET_ANDROID) || defined(TARGET_OF_IOS)
  all_fonts().erase(this);
#endif

  if (mImpl != NULL)
    delete mImpl;
}

void ofxTrueTypeFontUC::Impl::implUnloadFont() {
  if(!bLoadedOk_)
    return;

  cps.clear();
  textures.clear();
  loadedChars.clear();

  // ------------- close the library and typeface
  FT_Done_Face(face_);
  FT_Done_FreeType(library_);

  bLoadedOk_ = false;
}

void ofxTrueTypeFontUC::unloadFont() {
  mImpl->implUnloadFont();
}

bool ofxTrueTypeFontUC::Impl::loadFontFace(string fontname){
  filename_ = ofToDataPath(fontname,true);
  ofFile fontFile(filename_, ofFile::Reference);
  int fontID = 0;
  if (!fontFile.exists()) {
#ifdef TARGET_LINUX
    filename_ = linuxFontPathByName(fontname);
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
    filename_ = osxFontPathByName(fontname);
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
    filename_ = winFontPathByName(fontname);
#endif
    if (filename_ == "" ) {
      ofLogError("ofxTrueTypeFontUC") << "loadFontFace(): couldn't find font \"" << fontname << "\"";
      return false;
    }
    ofLogVerbose("ofxTrueTypeFontUC") << "loadFontFace(): \"" << fontname << "\" not a file in data loading system font from \"" << filename_ << "\"";
  }
  FT_Error err;
  err = FT_New_Face( library_, filename_.c_str(), fontID, &face_ );
  if (err) {
    // simple error table in lieu of full table (see fterrors.h)
    string errorString = "unknown freetype";
    if(err == 1) errorString = "INVALID FILENAME";
    ofLogError("ofxTrueTypeFontUC") << "loadFontFace(): couldn't create new face for \"" << fontname << "\": FT_Error " << err << " " << errorString;
    return false;
  }

  return true;
}

void ofxTrueTypeFontUC::reloadFont() {
  mImpl->implLoadFont(mImpl->filename_, mImpl->fontSize_, mImpl->bAntiAliased_, mImpl->bMakeContours_, mImpl->simplifyAmt_, mImpl->dpi_);
}

//-----------------------------------------------------------
bool ofxTrueTypeFontUC::load(string filename, int fontsize, bool bAntiAliased, bool makeContours, float simplifyAmt, int dpi) {
  return loadFont(filename, fontsize, bAntiAliased, makeContours, simplifyAmt, dpi);
}

bool ofxTrueTypeFontUC::loadFont(string filename, int fontsize, bool bAntiAliased, bool makeContours, float simplifyAmt, int dpi) {
  return mImpl->implLoadFont(filename, fontsize, bAntiAliased, makeContours, simplifyAmt, dpi);

}

bool ofxTrueTypeFontUC::Impl::implLoadFont(string filename, int fontsize, bool bAntiAliased, bool makeContours, float simplifyAmt, int dpi) {
  bMakeContours_ = makeContours;

  //------------------------------------------------
  if (bLoadedOk_ == true){
    // we've already been loaded, try to clean up :
    implUnloadFont();
  }
  //------------------------------------------------

  dpi_ = dpi;
  if( dpi_ == 0 ){
    dpi_ = ttfGlobalDpi_;
  }

  filename_ = ofToDataPath(filename);

  bLoadedOk_ = false;
  bAntiAliased_ = bAntiAliased;
  fontSize_ = fontsize;
  simplifyAmt_ = simplifyAmt;

  //--------------- load the library and typeface
  FT_Error err = FT_Init_FreeType(&library_);
  if (err) {
    ofLog(OF_LOG_ERROR,"ofxTrueTypeFontUC::loadFont - Error initializing freetype lib: FT_Error = %d", err);
    return false;
  }
  err = FT_New_Face(library_, filename_.c_str(), 0, &face_);
  if (err) {
    // simple error table in lieu of full table (see fterrors.h)
    string errorString = "unknown freetype";
    if (err == 1)
      errorString = "INVALID FILENAME";
    ofLog(OF_LOG_ERROR,"ofxTrueTypeFontUC::loadFont - %s: %s: FT_Error = %d", errorString.c_str(), filename_.c_str(), err);
    return false;
  }

  FT_Set_Char_Size(face_, fontSize_ << 6, fontSize_ << 6, dpi_, dpi_);
  lineHeight_ = fontSize_ * 1.43f;

  //------------------------------------------------------
  //kerning would be great to support:
  //ofLog(OF_LOG_NOTICE,"FT_HAS_KERNING ? %i", FT_HAS_KERNING(face));
  //------------------------------------------------------

  implReserveCharacters(limitCharactersNum_);

  bLoadedOk_ = true;
  return true;
}

//-----------------------------------------------------------
bool ofxTrueTypeFontUC::isLoaded() {
  return mImpl->bLoadedOk_;
}

//-----------------------------------------------------------
bool ofxTrueTypeFontUC::isAntiAliased() {
  return mImpl->bAntiAliased_;
}

//-----------------------------------------------------------
int ofxTrueTypeFontUC::getFontSize() {
  return mImpl->fontSize_;
}

//-----------------------------------------------------------
void ofxTrueTypeFontUC::setLineHeight(float _newLineHeight) {
  mImpl->lineHeight_ = _newLineHeight;
}

//-----------------------------------------------------------
float ofxTrueTypeFontUC::getLineHeight() {
  return mImpl->lineHeight_;
}

//-----------------------------------------------------------
void ofxTrueTypeFontUC::setLetterSpacing(float _newletterSpacing) {
  mImpl->letterSpacing_ = _newletterSpacing;
}

//-----------------------------------------------------------
float ofxTrueTypeFontUC::getLetterSpacing() {
  return mImpl->letterSpacing_;
}

//-----------------------------------------------------------
void ofxTrueTypeFontUC::setSpaceSize(float _newspaceSize) {
  mImpl->spaceSize_ = _newspaceSize;
}

//-----------------------------------------------------------
float ofxTrueTypeFontUC::getSpaceSize() {
  return mImpl->spaceSize_;
}

ofPath ofxTrueTypeFontUC::Impl::getCharacterAsPointsFromCharID(const int &charID) {
  if (bMakeContours_ == false)
    ofLog(OF_LOG_ERROR, "getCharacterAsPoints: contours not created,  call loadFont with makeContours set to true");

  if (bMakeContours_ && (int)charOutlines.size() > 0)
    return charOutlines[charID];
  else {
    if(charOutlines.empty())
      charOutlines.push_back(ofPath());
    return charOutlines[0];
  }
}

//-----------------------------------------------------------
void ofxTrueTypeFontUC::Impl::drawChar(int c, float x, float y) {

  if (c >= limitCharactersNum_) {
    //ofLog(OF_LOG_ERROR,"Error : char (%i) not allocated -- line %d in %s", (c + NUM_CHARACTER_TO_START), __LINE__,__FILE__);
    return;
  }

  GLfloat	x1, y1, x2, y2;
  GLfloat t1, v1, t2, v2;
  t2 = cps[c].t2;
  v2 = cps[c].v2;
  t1 = cps[c].t1;
  v1 = cps[c].v1;

  x1 = cps[c].x1+x;
  y1 = cps[c].y1+y;
  x2 = cps[c].x2+x;
  y2 = cps[c].y2+y;

  int firstIndex = stringQuads.getVertices().size();

  stringQuads.addVertex(ofVec3f(x1,y1));
  stringQuads.addVertex(ofVec3f(x2,y1));
  stringQuads.addVertex(ofVec3f(x2,y2));
  stringQuads.addVertex(ofVec3f(x1,y2));

  stringQuads.addTexCoord(ofVec2f(t1,v1));
  stringQuads.addTexCoord(ofVec2f(t2,v1));
  stringQuads.addTexCoord(ofVec2f(t2,v2));
  stringQuads.addTexCoord(ofVec2f(t1,v2));

  stringQuads.addIndex(firstIndex);
  stringQuads.addIndex(firstIndex+1);
  stringQuads.addIndex(firstIndex+2);
  stringQuads.addIndex(firstIndex+2);
  stringQuads.addIndex(firstIndex+3);
  stringQuads.addIndex(firstIndex);
}

//-----------------------------------------------------------
vector<ofPath> ofxTrueTypeFontUC::getStringAsPoints(const string &src, bool vflip){
  vector<ofPath> shapes;

  if (!mImpl->bLoadedOk_) {
    ofLog(OF_LOG_ERROR,"Error : font not allocated -- line %d in %s", __LINE__,__FILE__);
    return shapes;
  }

  GLint index	= 0;
  GLfloat X = 0;
  GLfloat Y = 0;
  int newLineDirection = 1;

  if (!vflip) {
    // this would align multiline texts to the last line when vflip is disabled
    //int lines = ofStringTimesInString(c,"\n");
    //Y = lines*lineHeight;
    newLineDirection = -1;
  }

  basic_string<unsigned int> utf32_src = convToUTF32(src);
  int len = (int)utf32_src.length();
  int c, cy;

  while (index < len) {
      c = utf32_src[index];
      if (c == '\n' || (mMaxWidth > 0 && X >= mMaxWidth)) {
          Y += mImpl->lineHeight_ * newLineDirection;
          X = 0;
      }
      else if (c == ' ') {
          cy = mImpl->getCharID('p');
          X += mImpl->cps[cy].setWidth * mImpl->letterSpacing_ * mImpl->spaceSize_;
      }
      else {
          cy = mImpl->getCharID(c);
          if (mImpl->cps[cy].character == mImpl->kTypefaceUnloaded)
              mImpl->loadChar(cy);
          shapes.push_back(mImpl->getCharacterAsPointsFromCharID(cy));
          shapes.back().translate(ofPoint(X,Y));
          X += mImpl->cps[cy].setWidth * mImpl->letterSpacing_;
      }
    index++;
  }

  return shapes;
}

//-----------------------------------------------------------
void ofxTrueTypeFontUC::Impl::drawCharAsShape(int c, float x, float y) {
  if (c >= limitCharactersNum_) {
    //ofLog(OF_LOG_ERROR,"Error : char (%i) not allocated -- line %d in %s", (c + NUM_CHARACTER_TO_START), __LINE__,__FILE__);
    return;
  }
  //-----------------------

  int cu = c;
  ofPath & charRef = charOutlines[cu];
  charRef.setFilled(ofGetStyle().bFill);
  charRef.draw(x,y);
}

ofRectangle ofxTrueTypeFontUC::getStringBoundingBox(const string &src, float x, float y){
  ofRectangle myRect;

  if (!mImpl->bLoadedOk_) {
    ofLog(OF_LOG_ERROR,"ofxTrueTypeFontUC::getStringBoundingBox - font not allocated");
    return myRect;
  }

  basic_string<unsigned int> utf32_src = convToUTF32(src);
  int len = (int)utf32_src.length();

  GLint index = 0;
  GLfloat xoffset	= 0;
  GLfloat yoffset	= 0;
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
  int c, cy;

  while (index < len)
  {
      c = utf32_src[index];
      if (c == '\n' || (mMaxWidth > 0 && xoffset >= mMaxWidth)) {
          yoffset += mImpl->lineHeight_;
          xoffset = 0 ; //reset X Pos back to zero
      }
      else if (c == ' ') {
          cy = mImpl->getCharID('p');
          xoffset += mImpl->cps[cy].width * mImpl->letterSpacing_ * mImpl->spaceSize_;
          // zach - this is a bug to fix -- for now, we don't currently deal with ' ' in calculating string bounding box
      }
      else {
          cy = mImpl->getCharID(c);
          if (mImpl->cps[cy].character == mImpl->kTypefaceUnloaded)
              mImpl->loadChar(cy);
          GLint height = mImpl->cps[cy].height;
          GLint bwidth = mImpl->cps[cy].width * mImpl->letterSpacing_;
          GLint top = mImpl->cps[cy].topExtent - mImpl->cps[cy].height;
          GLint lextent	= mImpl->cps[cy].leftExtent;
          float	x1, y1, x2, y2, corr, stretch;
          stretch = 0;
          corr = (float)(((mImpl->fontSize_ - height) + top) - mImpl->fontSize_);
          x1 = (x + xoffset + lextent + bwidth + stretch);
          y1 = (y + yoffset + height + corr + stretch);
          x2 = (x + xoffset + lextent);
          y2 = (y + yoffset + -top + corr);
          xoffset += mImpl->cps[cy].setWidth * mImpl->letterSpacing_;
          if (bFirstCharacter == true) {
              minx = x2;
              miny = y2;
              maxx = x1;
              maxy = y1;
              bFirstCharacter = false;
          }
          else {
              if (x2 < minx)
                  minx = x2;
              if (y2 < miny)
                  miny = y2;
              if (x1 > maxx)
                  maxx = x1;
              if (y1 > maxy)
                  maxy = y1;
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

float ofxTrueTypeFontUC::stringWidth(const string &str) {
    ofRectangle rect = getStringBoundingBox(str, 0,0);
    return rect.width;
}

float ofxTrueTypeFontUC::stringHeight(const string &str) {
    ofRectangle rect = getStringBoundingBox(str, 0,0);
    return rect.height;
}



//=====================================================================
void ofxTrueTypeFontUC::drawString(const string &src, float x, float y){
  if (!mImpl->bLoadedOk_) {
    ofLog(OF_LOG_ERROR,"ofxTrueTypeFontUC::drawString - Error : font not allocated -- line %d in %s", __LINE__,__FILE__);
    return;
  }

  GLint index	= 0;
  GLfloat X = x;
  GLfloat Y = y;

  basic_string<unsigned int> utf32_src = convToUTF32(src);
  int len = (int)utf32_src.length();
  int c, cy;

  while (index < len) {
      c = utf32_src[index];
      if (c == '\n' || (mMaxWidth > 0 && X >= mMaxWidth)) {
          Y += mImpl->lineHeight_;
          X = x ; //reset X Pos back to zero
      }
      else if (c == ' ') {
          int cy = mImpl->getCharID('p');
          X += mImpl->cps[cy].width * mImpl->letterSpacing_ * mImpl->spaceSize_;
      }
      else {
          cy = mImpl->getCharID(c);
          if (mImpl->cps[cy].character == mImpl->kTypefaceUnloaded)
              mImpl->loadChar(cy);
          mImpl->bind(cy);
          mImpl->drawChar(cy, X, Y);
          mImpl->unbind(cy);
          X += mImpl->cps[cy].setWidth * mImpl->letterSpacing_;
      }
    index++;
  }
}

//=====================================================================
void ofxTrueTypeFontUC::drawStringAsShapes(const string &src, float x, float y){

  if (!mImpl->bLoadedOk_) {
    ofLogError("ofxTrueTypeFontUC") << "drawStringAsShapes(): font not allocated: line " << __LINE__ << " in " << __FILE__;
    return;
  }

  //----------------------- error checking
  if (!mImpl->bMakeContours_) {
    ofLog(OF_LOG_ERROR,"ofxTrueTypeFontUC::drawStringAsShapes - Error : contours not created for this font - call loadFont with makeContours set to true");
    return;
  }

  GLint index = 0;
  GLfloat X = x;
  GLfloat Y = y;

  basic_string<unsigned int> utf32_src = convToUTF32(src);
  int len = (int)utf32_src.length();

  int c, cy;

  while (index < len)
  {
      c = utf32_src[index];
      if (c == '\n' || (mMaxWidth > 0 && X >= mMaxWidth)) {
          Y += mImpl->lineHeight_;
          X = x ; //reset X Pos back to zero
      }
      else if (c == ' ') {
          cy = mImpl->getCharID('p');
          X += mImpl->cps[cy].width;
      }
      else {
          cy = mImpl->getCharID(c);
          if (mImpl->cps[cy].character == mImpl->kTypefaceUnloaded)
              mImpl->loadChar(cy);
          mImpl->drawCharAsShape(cy, X, Y);
          X += mImpl->cps[cy].setWidth;
      }
      index++;
  }
}

//-----------------------------------------------------------
int ofxTrueTypeFontUC::getLoadedCharactersCount() {
  return mImpl->loadedChars.size();
}

//=====================================================================
const int ofxTrueTypeFontUC::Impl::kTypefaceUnloaded = 0;
const int ofxTrueTypeFontUC::Impl::kDefaultLimitCharactersNum = 10000;

//-----------------------------------------------------------
void ofxTrueTypeFontUC::Impl::bind(const int & charID) {
  if (!binded_) {
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
#ifndef TARGET_OPENGLES
    glPushAttrib(GL_COLOR_BUFFER_BIT);
#else
    blend_enabled_ = glIsEnabled(GL_BLEND);
    texture_2d_enabled_ = glIsEnabled(GL_TEXTURE_2D);
    glGetIntegerv( GL_BLEND_SRC, &blend_src);
    glGetIntegerv( GL_BLEND_DST, &blend_dst_ );
#endif

    // (b) enable our regular ALPHA blending!
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    textures[charID].bind();
    stringQuads.clear();
    binded_ = true;
  }
}

//-----------------------------------------------------------
void ofxTrueTypeFontUC::Impl::unbind(const int & charID) {
  if (binded_) {
    stringQuads.drawFaces();
    textures[charID].unbind();

#ifndef TARGET_OPENGLES
    glPopAttrib();
#else
    if (!blend_enabled_)
      glDisable(GL_BLEND);
    if  (!texture_2d_enabled_)
      glDisable(GL_TEXTURE_2D);
    glBlendFunc( blend_src, blend_dst_ );
#endif
    binded_ = false;
  }
}

//-----------------------------------------------------------
int ofxTrueTypeFontUC::getLimitCharactersNum() {
  return mImpl->limitCharactersNum_;
}

//-----------------------------------------------------------
void ofxTrueTypeFontUC::reserveCharacters(int charactersNumber) {
  mImpl->implReserveCharacters(charactersNumber);
}

void ofxTrueTypeFontUC::setMaxWidth(unsigned int maxWidth) {
    mMaxWidth = maxWidth;
}

unsigned int ofxTrueTypeFontUC::getMaxWidth() const {
    return mMaxWidth;
}

void ofxTrueTypeFontUC::Impl::implReserveCharacters(int num) {
  if (num <= 0)
    return;

  limitCharactersNum_ = num;

  vector<charPropsUC>().swap(cps);
  vector<ofTexture>().swap(textures);
  vector<int>().swap(loadedChars);
  vector<ofPath>().swap(charOutlines);

  //--------------- initialize character info and textures
  cps.resize(limitCharactersNum_);
  for (int i=0; i<limitCharactersNum_; ++i)
    cps[i].character = kTypefaceUnloaded;

  textures.resize(limitCharactersNum_);

  if (bMakeContours_) {
    charOutlines.clear();
    charOutlines.assign(limitCharactersNum_, ofPath());
  }

  //--------------- load 'p' character for display ' '
  int cy = getCharID('p');
  loadChar(cy);
}

//-----------------------------------------------------------
int ofxTrueTypeFontUC::Impl::getCharID(const int &c) {
  int tmp = (int)c;
  int point = 0;
  for (; point != (int)loadedChars.size(); ++point) {
    if (loadedChars[point] == tmp) {
      break;
    }
  }
  if (point == loadedChars.size()) {
    //----------------------- error checking
    if (point >= limitCharactersNum_) {
      ofLog(OF_LOG_ERROR,"ofxTrueTypeFontUC::getCharID - Error : too many typeface already loaded - call loadFont to reset");
      return point = 0;
    }
    else {
      loadedChars.push_back(tmp);
    }
  }
  return point;
}

//-----------------------------------------------------------
void ofxTrueTypeFontUC::Impl::loadChar(const int &charID) {
  int i = charID;
  ofPixels expandedData;

  //------------------------------------------ anti aliased or not:
  FT_Error err = FT_Load_Glyph( face_, FT_Get_Char_Index( face_, loadedChars[i] ), FT_LOAD_DEFAULT );
  if(err)
    ofLog(OF_LOG_ERROR,"ofxTrueTypeFontUC::loadFont - Error with FT_Load_Glyph %i: FT_Error = %d", loadedChars[i], err);

  if (bAntiAliased_ == true)
    FT_Render_Glyph(face_->glyph, FT_RENDER_MODE_NORMAL);
  else
    FT_Render_Glyph(face_->glyph, FT_RENDER_MODE_MONO);

  //------------------------------------------
  FT_Bitmap& bitmap= face_->glyph->bitmap;

  // prepare the texture:
  /*int width  = ofNextPow2( bitmap.width + border*2 );
   int height = ofNextPow2( bitmap.rows  + border*2 );


   // ------------------------- this is fixing a bug with small type
   // ------------------------- appearantly, opengl has trouble with
   // ------------------------- width or height textures of 1, so we
   // ------------------------- we just set it to 2...
   if (width == 1) width = 2;
   if (height == 1) height = 2;*/

  if (bMakeContours_) {
    if (printVectorInfo_)
      printf("\n\ncharacter charID %d: \n", i );

    charOutlines[i] = makeContoursForCharacter(face_);
    if (simplifyAmt_>0)
      charOutlines[i].simplify(simplifyAmt_);
    charOutlines[i].getTessellation();
  }

  // -------------------------
  // info about the character:
  cps[i].character = loadedChars[i];
  cps[i].height = face_->glyph->bitmap_top;
  cps[i].width = face_->glyph->bitmap.width;
  cps[i].setWidth = face_->glyph->advance.x >> 6;
  cps[i].topExtent = face_->glyph->bitmap.rows;
  cps[i].leftExtent = face_->glyph->bitmap_left;

  int width = cps[i].width;
  int height = bitmap.rows;

  cps[i].tW = width;
  cps[i].tH = height;

  GLint fheight = cps[i].height;
  GLint bwidth = cps[i].width;
  GLint top = cps[i].topExtent - cps[i].height;
  GLint lextent	= cps[i].leftExtent;

  GLfloat	corr, stretch;

  //this accounts for the fact that we are showing 2*visibleBorder extra pixels
  //so we make the size of each char that many pixels bigger
  stretch = 0;

  corr	= (float)(((fontSize_ - fheight) + top) - fontSize_);

  cps[i].x1 = lextent + bwidth + stretch;
  cps[i].y1 = fheight + corr + stretch;
  cps[i].x2 = (float) lextent;
  cps[i].y2 = -top + corr;

  // Allocate Memory For The Texture Data.
  expandedData.allocate(width, height, 2);
  //-------------------------------- clear data:
  expandedData.set(0,255); // every luminance pixel = 255
  expandedData.set(1,0);

  if (bAntiAliased_ == true) {
    ofPixels bitmapPixels;
    bitmapPixels.setFromExternalPixels(bitmap.buffer,bitmap.width,bitmap.rows,1);
    expandedData.setChannel(1,bitmapPixels);
  }
  else {
    //-----------------------------------
    // true type packs monochrome info in a
    // 1-bit format, hella funky
    // here we unpack it:
    unsigned char *src =  bitmap.buffer;
    for(int j=0; j<bitmap.rows; ++j) {
      unsigned char b=0;
      unsigned char *bptr =  src;
      for(int k=0; k<bitmap.width; ++k){
        expandedData[2*(k+j*width)] = 255;

        if (k%8==0)
          b = (*bptr++);

        expandedData[2*(k+j*width) + 1] = b&0x80 ? 255 : 0;
        b <<= 1;
      }
      src += bitmap.pitch;
    }
    //-----------------------------------
  }

  int longSide = border_ * 2;
  cps[i].tW > cps[i].tH ? longSide += cps[i].tW : longSide += cps[i].tH;

  int tmp = 1;
  while (longSide > tmp) {
    tmp <<= 1;
  }
  int w = tmp;
  int h = w;

  ofPixels atlasPixels;
  atlasPixels.allocate(w,h,2);
  atlasPixels.set(0,255);
  atlasPixels.set(1,0);

  cps[i].t2 = float(border_) / float(w);
  cps[i].v2 = float(border_) / float(h);
  cps[i].t1 = float(cps[i].tW + border_) / float(w);
  cps[i].v1 = float(cps[i].tH + border_) / float(h);
  expandedData.pasteInto(atlasPixels, border_, border_);

  textures[i].allocate(atlasPixels.getWidth(), atlasPixels.getHeight(), GL_LUMINANCE_ALPHA, false);

  if (bAntiAliased_ && fontSize_>20) {
    textures[i].setTextureMinMagFilter(GL_LINEAR,GL_LINEAR);
  }
  else {
    textures[i].setTextureMinMagFilter(GL_NEAREST,GL_NEAREST);
  }

  #if (OF_VERSION_MAJOR == 0 && OF_VERSION_MINOR >= 9 && OF_VERSION_PATCH >= 2) || OF_VERSION_MAJOR > 0
    textures[i].loadData(atlasPixels.getData(), atlasPixels.getWidth(), atlasPixels.getHeight(), GL_LUMINANCE_ALPHA);
  #else
    textures[i].loadData(atlasPixels.getPixels(), atlasPixels.getWidth(), atlasPixels.getHeight(), GL_LUMINANCE_ALPHA);
  #endif
}

