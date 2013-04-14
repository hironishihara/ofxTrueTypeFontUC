#include "ofxTrueTypeFontUC.h"
//--------------------------

#include "ft2build.h"
#include "freetype2/freetype/freetype.h"
#include "freetype2/freetype/ftglyph.h"
#include "freetype2/freetype/ftoutln.h"
#include "freetype2/freetype/fttrigon.h"

#include <algorithm>

#ifdef TARGET_WIN32
#include <locale>
#endif

#include "ofPoint.h"
#include "ofConstants.h"
#include "ofTexture.h"
#include "ofMesh.h"
#include "ofUtils.h"
#include "ofGraphics.h"

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
  Impl() {};
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
  string filename;
  
  // ofTexture texAtlas;
  vector<ofTexture> textures;
  bool binded_;
  ofMesh stringQuads;
  
  typedef struct FT_LibraryRec_ * FT_Library;
  typedef struct FT_FaceRec_ * FT_Face;
  FT_Library library;
  FT_Face face;
  
  ofPath getCharacterAsPointsFromCharID(const int & charID);
  
  void bind(const int & charID);
  void unbind(const int & charID);
  
  int getCharID(const int & c);
  void loadChar(const int & charID);
  vector<int> loadedChars;
  
  static const int kNumCharcterToStart;
  static const int kTypefaceUnloaded;
  static const int kDefaultLimitCharactersNum;
  
#ifdef TARGET_OPENGLES
  GLint blend_src, blend_dst_;
  GLboolean blend_enabled_;
  GLboolean texture_2d_enabled_;
#endif
};

static bool printVectorInfo_ = false;
static int ttfGlobalDpi_ = 96;

//--------------------------------------------------------

namespace util {
  namespace ofxTrueTypeFontUC {

template <class T>
wstring convToUCS4(basic_string<T> src) {
  wstring dst = L"";
  
  // convert UTF-8 on char or wchar_t to UCS-4 on wchar_t
  int size = src.size();
  int index = 0;
  while (index < size) {
    wchar_t c = (unsigned char)src[index];
    if (c < 0x80) {
      dst += (c);
    }
    else if (c < 0xe0) {
      if (index + 1 < size) {
        dst += (((c & 0x1f) << 6) | (src[index+1] & 0x3f));
        index++;
      }
    }
    else if (c < 0xf0) {
      if (index + 2 < size) {
        dst += (((c & 0x0f) << 12) | ((src[index+1] & 0x3f) << 6) |
                (src[index+2] & 0x3f));
        index += 2;
      }
    }
    else if (c < 0xf8) {
      if (index + 3 < size) {
        dst += (((c & 0x07) << 18) | ((src[index+1] & 0x3f) << 12) |
                ((src[index+2] & 0x3f) << 6) | (src[index+3] & 0x3f));
        index += 3;
      }
    }
    else if (c < 0xfc) {
      if (index + 4 < size) {
        dst += (((c & 0x03) << 24) | ((src[index+1] & 0x3f) << 18) |
                ((src[index+2] & 0x3f) << 12) | ((src[index+3] & 0x3f) << 6) |
                (src[index+4] & 0x3f));
        index += 4;
      }
    }
    else if (c < 0xfe) {
      if (index + 5 < size) {
        dst += (((c & 0x01) << 30) | ((src[index+1] & 0x3f) << 24) |
                ((src[index+2] & 0x3f) << 18) | ((src[index+3] & 0x3f) << 12) |
                ((src[index+4] & 0x3f) << 6) | (src[index+5] & 0x3f));
        index += 5;
      }
    }
    index++;
  }
  
  return dst;
}

#ifdef TARGET_WIN32

typedef basic_string<int> ustring;

ustring convUTF16ToUCS4(wstring src) {
  // decord surrogate pairs
  ustring dst;
  
  for (std::wstring::iterator iter=src.begin(); iter!=src.end(); ++iter) {
    if (0xD800 <=*iter && *iter<=0xDFFF) {
      if (0xD800<=*iter && *iter<=0xDBFF && 0xDC00<=*(iter+1) && *(iter+1)<=0xDFFF) {
        int hi = *iter & 0x3FF;
        int lo = *(iter+1) & 0x3FF;
        int code = (hi << 10) | lo;
        dst += code + 0x10000;
        ++iter;
      }
      else {
        // ofLog(OF_LOG_ERROR, "util::ofxTrueTypeFontUC::convUTF16ToUCS4 - wrong input" );
      }
    }
    else
      dst += *iter;
  }
  
  return dst;
}

#endif

wstring convToWString(string src) {
  
#ifdef TARGET_WIN32
  wstring dst = L"";
  typedef codecvt<wchar_t, char, mbstate_t> codecvt_t;
  
  locale loc = locale("");
  if(!std::has_facet<codecvt_t>(loc))
    return dst;
  
  const codecvt_t & conv = use_facet<codecvt_t>(loc);
  
  const std::size_t size = src.length();
  std::vector<wchar_t> dst_vctr(size);
  
  if (dst_vctr.size() == 0)
    return dst;
  
  wchar_t * const buf = &dst_vctr[0];
  
  const char * dummy;
  wchar_t * next;
  mbstate_t state = {0};
  const char * const s = src.c_str();
  
  if (conv.in(state, s, s + size, dummy, buf, buf + size, next) == codecvt_t::ok)
    dst = std::wstring(buf, next - buf);
  
  return dst;
  
#elif defined __clang__
  wstring dst = L"";
  for (int i=0; i<src.size(); ++i)
    dst += src[i];
#if defined(__clang_major__) && (__clang_major__ >= 4)
  dst = convToUCS4<wchar_t>(dst);
#endif
  return dst;
#else
  return convToUCS4<char>(src);
#endif
}

  } // ofxTrueTypeFontUC
} // util

//--------------------------------------------------------
void ofxTrueTypeFontUC::setGlobalDpi(int newDpi){
  ttfGlobalDpi_ = newDpi;
}

//--------------------------------------------------------
static ofPath makeContoursForCharacter(FT_Face & face);

static ofPath makeContoursForCharacter(FT_Face & face) {  
  int nContours	= face->glyph->outline.n_contours;
  int startPos = 0;
  
  char * tags = face->glyph->outline.tags;
  FT_Vector * vec = face->glyph->outline.points;
  
  ofPath charOutlines;
  charOutlines.setUseShapeColor(false);
  
  for (int k=0; k<nContours; ++k) {
    if (k>0)
      startPos = face->glyph->outline.contours[k-1]+1;
    
    int endPos = face->glyph->outline.contours[k]+1;
    
    if (printVectorInfo_)
      printf("--NEW CONTOUR\n\n");
    
    //vector <ofPoint> testOutline;
    ofPoint lastPoint;
    
    for (int j=startPos; j<endPos; ++j) {
      
      if(FT_CURVE_TAG(tags[j]) == FT_CURVE_TAG_ON) {
        lastPoint.set((float)vec[j].x, (float)-vec[j].y, 0);
        if (printVectorInfo_)
          printf("flag[%i] is set to 1 - regular point - %f %f \n", j, lastPoint.x, lastPoint.y);
        
        charOutlines.lineTo(lastPoint/64);
      }
      else {
        if (printVectorInfo_)
          printf("flag[%i] is set to 0 - control point \n", j);
        
        if (FT_CURVE_TAG(tags[j]) == FT_CURVE_TAG_CUBIC) {
          if (printVectorInfo_)
            printf("- bit 2 is set to 2 - CUBIC\n");
          
          int prevPoint = j-1;
          if (j == 0)
            prevPoint = endPos-1;
          
          int nextIndex = j+1;
          if (nextIndex >= endPos)
            nextIndex = startPos;
          
          ofPoint nextPoint((float)vec[nextIndex].x,  -(float)vec[nextIndex].y);
          
          //we need two control points to draw a cubic bezier
          bool lastPointCubic =  (FT_CURVE_TAG(tags[prevPoint]) != FT_CURVE_TAG_ON) && (FT_CURVE_TAG(tags[prevPoint]) == FT_CURVE_TAG_CUBIC);
          
          if (lastPointCubic) {
            ofPoint controlPoint1((float)vec[prevPoint].x,	(float)-vec[prevPoint].y);
            ofPoint controlPoint2((float)vec[j].x, (float)-vec[j].y);
            ofPoint nextPoint((float) vec[nextIndex].x,	-(float) vec[nextIndex].y);
            
            charOutlines.bezierTo(controlPoint1.x/64, controlPoint1.y/64, controlPoint2.x/64, controlPoint2.y/64, nextPoint.x/64, nextPoint.y/64);
          }
        }
        else {
          ofPoint conicPoint((float)vec[j].x,  -(float)vec[j].y);
          
          if (printVectorInfo_)
            printf("- bit 2 is set to 0 - conic- \n");
          if (printVectorInfo_)
            printf("--- conicPoint point is %f %f \n", conicPoint.x, conicPoint.y);
          
          //If the first point is connic and the last point is connic then we need to create a virutal point which acts as a wrap around
          if (j == startPos) {
            bool prevIsConnic = (FT_CURVE_TAG(tags[endPos-1]) != FT_CURVE_TAG_ON) && (FT_CURVE_TAG(tags[endPos-1]) != FT_CURVE_TAG_CUBIC);
            
            if (prevIsConnic) {
              ofPoint lastConnic((float)vec[endPos - 1].x, (float)-vec[endPos - 1].y);
              lastPoint = (conicPoint + lastConnic) / 2;
              
              if (printVectorInfo_)
                printf("NEED TO MIX WITH LAST\n");
              if(printVectorInfo_)
                printf("last is %f %f \n", lastPoint.x, lastPoint.y);
            }
          }
          
          int nextIndex = j+1;
          if (nextIndex >= endPos)
            nextIndex = startPos;
          
          ofPoint nextPoint((float)vec[nextIndex].x, -(float)vec[nextIndex].y);
          
          if (printVectorInfo_)
            printf("--- last point is %f %f \n", lastPoint.x, lastPoint.y);
          
          bool nextIsConnic = (FT_CURVE_TAG(tags[nextIndex]) != FT_CURVE_TAG_ON) && (FT_CURVE_TAG(tags[nextIndex]) != FT_CURVE_TAG_CUBIC);
          
          //create a 'virtual on point' if we have two connic points
          if (nextIsConnic) {
            nextPoint = (conicPoint + nextPoint) / 2;
            if(printVectorInfo_ )
              printf("|_______ double connic!\n");
          }
          if (printVectorInfo_)
            printf("--- next point is %f %f \n", nextPoint.x, nextPoint.y);
          
          charOutlines.quadBezierTo(lastPoint.x/64, lastPoint.y/64, conicPoint.x/64, conicPoint.y/64, nextPoint.x/64, nextPoint.y/64);
          
          if (nextIsConnic) {
            lastPoint = nextPoint;
          }
        }
      }
      
    }
    
    charOutlines.close();
  }
  
  return charOutlines;
}

#if defined(TARGET_ANDROID) || defined(TARGET_OF_IPHONE)
#include <set>
static set<ofTrueTypeFont*> & all_fonts() {
  static set<ofTrueTypeFont*> *all_fonts = new set<ofTrueTypeFont*>;
  return *all_fonts;
}

void ofUnloadAllFontTextures() {
  set<ofTrueTypeFont*>::iterator iter;
  for(iter=all_fonts().begin(); iter!=all_fonts().end(); ++iter)
    (*iter)->unloadFont();
}

void ofReloadAllFontTextures() {
  set<ofTrueTypeFont*>::iterator iter;
  for(iter=all_fonts().begin(); iter!=all_fonts().end(); ++iter)
    (*iter)->reloadFont();
}
#endif

bool compare_cps(const charPropsUC & c1, const charPropsUC & c2) {
  if (c1.tH == c2.tH)
    return c1.tW > c2.tW;
  else
    return c1.tH > c2.tH;
}

//------------------------------------------------------------------
ofxTrueTypeFontUC::ofxTrueTypeFontUC() {
  mImpl = new Impl();
  mImpl->bLoadedOk_	= false;
  mImpl->bMakeContours_	= false;
#if defined(TARGET_ANDROID) || defined(TARGET_OF_IPHONE)
  all_fonts.insert(this);
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
  
#if defined(TARGET_ANDROID) || defined(TARGET_OF_IPHONE)
  all_fonts.erase(this);
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
  FT_Done_Face(face);
  FT_Done_FreeType(library);
  
  bLoadedOk_ = false;
}

void ofxTrueTypeFontUC::unloadFont() {
  mImpl->implUnloadFont();
}

void ofxTrueTypeFontUC::reloadFont() {
  mImpl->implLoadFont(mImpl->filename, mImpl->fontSize_, mImpl->bAntiAliased_, mImpl->bMakeContours_, mImpl->simplifyAmt_, mImpl->dpi_);
}

//-----------------------------------------------------------
bool ofxTrueTypeFontUC::loadFont(string filename, int fontsize, bool _bAntiAliased, bool makeContours, float _simplifyAmt, int _dpi) {
  mImpl->implLoadFont(filename, fontsize, _bAntiAliased, makeContours, _simplifyAmt, _dpi);
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
  
  filename = ofToDataPath(filename);
  
  bLoadedOk_ = false;
  bAntiAliased_ = bAntiAliased;
  fontSize_ = fontsize;
  simplifyAmt_ = simplifyAmt;
  
  //--------------- load the library and typeface
  FT_Error err = FT_Init_FreeType(&library);
  if (err) {
    ofLog(OF_LOG_ERROR,"ofxTrueTypeFontUC::loadFont - Error initializing freetype lib: FT_Error = %d", err);
    return false;
  }
  err = FT_New_Face(library, filename.c_str(), 0, &face);
  if (err) {
    // simple error table in lieu of full table (see fterrors.h)
    string errorString = "unknown freetype";
    if (err == 1)
      errorString = "INVALID FILENAME";
    ofLog(OF_LOG_ERROR,"ofxTrueTypeFontUC::loadFont - %s: %s: FT_Error = %d", errorString.c_str(), filename.c_str(), err);
    return false;
  }
  
  FT_Set_Char_Size(face, fontSize_ << 6, fontSize_ << 6, dpi_, dpi_);
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

//------------------------------------------------------------------
ofPath ofxTrueTypeFontUC::getCharacterAsPoints(wstring src) {
  
#ifdef TARGET_WIN32
  ustring character = util::ofxTrueTypeFontUC::convUTF16ToUCS4(src);
#elif defined(__clang_major__) && (__clang_major__ <= 3)
  wstring character = util::ofxTrueTypeFontUC::convToUCS4<wchar_t>(src);
#else
  wstring character = src;
#endif
  
  int charID = mImpl->getCharID(character[0]);
  if(mImpl->cps[charID].character == mImpl->kTypefaceUnloaded)
    mImpl->loadChar(charID);
  
  return mImpl->getCharacterAsPointsFromCharID(charID);
}

ofPath ofxTrueTypeFontUC::getCharacterAsPoints(string character) {
  return getCharacterAsPoints(util::ofxTrueTypeFontUC::convToWString(character));
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
vector<ofPath> ofxTrueTypeFontUC::getStringAsPoints(wstring src) {
  
#ifdef TARGET_WIN32
  ustring s = util::ofxTrueTypeFontUC::convUTF16ToUCS4(src);
#elif defined(__clang_major__) && (__clang_major__ <= 3)
  wstring s = util::ofxTrueTypeFontUC::convToUCS4<wchar_t>(src);
#else
  wstring s = src;
#endif
  
  vector<ofPath> shapes;
  
  if (!mImpl->bLoadedOk_) {
    ofLog(OF_LOG_ERROR,"Error : font not allocated -- line %d in %s", __LINE__,__FILE__);
    return shapes;
  };
  
  GLint index	= 0;
  GLfloat X = 0;
  GLfloat Y = 0;
  
  int len = (int)s.length();
  
  while (index < len) {
    int cy = mImpl->getCharID(s[index]);
    if (mImpl->cps[cy].character == mImpl->kTypefaceUnloaded) {
      mImpl->loadChar(cy);
    }
    if (cy < mImpl->limitCharactersNum_) {  // full char set or not?
      if (s[index] == L'\n') {
        Y += mImpl->lineHeight_;
        X = 0 ; //reset X Pos back to zero
      }
      else if (s[index] == L' ') {
        int cy = mImpl->getCharID(L'p');
        X += mImpl->cps[cy].width * mImpl->letterSpacing_ * mImpl->spaceSize_;
      }
      else {
        shapes.push_back(mImpl->getCharacterAsPointsFromCharID(cy));
        shapes.back().translate(ofPoint(X,Y));
        X += mImpl->cps[cy].setWidth * mImpl->letterSpacing_;
      }
    }
    index++;
  }
  
  return shapes;
}

vector<ofPath> ofxTrueTypeFontUC::getStringAsPoints(string s) {
  return getStringAsPoints(util::ofxTrueTypeFontUC::convToWString(s));
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

//-----------------------------------------------------------
float ofxTrueTypeFontUC::stringWidth(wstring c) {
  ofRectangle rect = getStringBoundingBox(c, 0,0);
  return rect.width;
}

float ofxTrueTypeFontUC::stringWidth(string s) {
  return stringWidth(util::ofxTrueTypeFontUC::convToWString(s));
}


ofRectangle ofxTrueTypeFontUC::getStringBoundingBox(wstring src, float x, float y) {
  
#ifdef TARGET_WIN32
  ustring c = util::ofxTrueTypeFontUC::convUTF16ToUCS4(src);
#elif defined(__clang_major__) && (__clang_major__ <= 3)
  wstring c = util::ofxTrueTypeFontUC::convToUCS4<wchar_t>(src);
#else
  wstring c = src;
#endif
  
  ofRectangle myRect;
  
  if (!mImpl->bLoadedOk_) {
    ofLog(OF_LOG_ERROR,"ofxTrueTypeFontUC::getStringBoundingBox - font not allocated");
    return myRect;
  }
  
  GLint index = 0;
  GLfloat xoffset	= 0;
  GLfloat yoffset	= 0;
  int len = (int)c.length();
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
    int cy = mImpl->getCharID(c[index]);
    if(mImpl->cps[cy].character == mImpl->kTypefaceUnloaded)
      mImpl->loadChar(cy);
    
    if (cy < mImpl->limitCharactersNum_) { // full char set or not?
      if (c[index] == L'\n') {
        yoffset += mImpl->lineHeight_;
        xoffset = 0 ; //reset X Pos back to zero
      }
      else if (c[index] == L' ') {
        // int cy = (int)'p' - NUM_CHARACTER_TO_START;
        int cy = mImpl->getCharID(L'p');
        xoffset += mImpl->cps[cy].width * mImpl->letterSpacing_ * mImpl->spaceSize_;
        // zach - this is a bug to fix -- for now, we don't currently deal with ' ' in calculating string bounding box
      }
      else {
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
    }
    index++;
  }
  
  myRect.x = minx;
  myRect.y = miny;
  myRect.width = maxx-minx;
  myRect.height = maxy-miny;
  
  return myRect;
}

ofRectangle ofxTrueTypeFontUC::getStringBoundingBox(string s, float x, float y) {
  return getStringBoundingBox(util::ofxTrueTypeFontUC::convToWString(s), x, y);
}

//-----------------------------------------------------------
float ofxTrueTypeFontUC::stringHeight(wstring c) {
  ofRectangle rect = getStringBoundingBox(c, 0,0);
  return rect.height;
}

float ofxTrueTypeFontUC::stringHeight(string s) {
  return stringHeight(util::ofxTrueTypeFontUC::convToWString(s));
}

//=====================================================================
void ofxTrueTypeFontUC::drawString(wstring src, float x, float y) {
  
#ifdef TARGET_WIN32
  ustring c = util::ofxTrueTypeFontUC::convUTF16ToUCS4(src);
#elif defined(__clang_major__) && (__clang_major__ <= 3)
  wstring c = util::ofxTrueTypeFontUC::convToUCS4<wchar_t>(src);
#else
  wstring c = src;
#endif
  
  /*glEnable(GL_BLEND);
   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
   texAtlas.draw(0,0);*/
  
  if (!mImpl->bLoadedOk_){
    ofLog(OF_LOG_ERROR,"ofxTrueTypeFontUC::drawString - Error : font not allocated -- line %d in %s", __LINE__,__FILE__);
    return;
  };
  
  GLint index	= 0;
  GLfloat X = x;
  GLfloat Y = y;
  
  bool alreadyBinded = mImpl->binded_;
  
  int len = (int)c.length();
  
  while (index < len) {
    int cy = mImpl->getCharID(c[index]);
    if (mImpl->cps[cy].character == mImpl->kTypefaceUnloaded)
      mImpl->loadChar(cy);
    
    if (cy < mImpl->limitCharactersNum_) { // full char set or not?
      if (c[index] == L'\n') {
        Y += mImpl->lineHeight_;
        X = x ; //reset X Pos back to zero
      }
      else if (c[index] == L' ') {
        int cy = mImpl->getCharID(L'p');
        X += mImpl->cps[cy].width * mImpl->letterSpacing_ * mImpl->spaceSize_;
      }
      else {
        mImpl->bind(cy);
        mImpl->drawChar(cy, X, Y);
        mImpl->unbind(cy);
        X += mImpl->cps[cy].setWidth * mImpl->letterSpacing_;
      }
    }
    
    index++;
  }
}

void ofxTrueTypeFontUC::drawString(string s, float x, float y) {
  return drawString(util::ofxTrueTypeFontUC::convToWString(s), x, y);
}

//=====================================================================
void ofxTrueTypeFontUC::drawStringAsShapes(wstring src, float x, float y) {
  if (!mImpl->bLoadedOk_) {
    ofLog(OF_LOG_ERROR,"ofxTrueTypeFontUC::drawStringAsShapes - Error : font not allocated -- line %d in %s", __LINE__,__FILE__);
    return;
  };
  
  //----------------------- error checking
  if (!mImpl->bMakeContours_) {
    ofLog(OF_LOG_ERROR,"ofxTrueTypeFontUC::drawStringAsShapes - Error : contours not created for this font - call loadFont with makeContours set to true");
    return;
  }
  
#ifdef TARGET_WIN32
  ustring c = util::ofxTrueTypeFontUC::convUTF16ToUCS4(src);
#elif defined(__clang_major__) && (__clang_major__ <= 3)
  wstring c = util::ofxTrueTypeFontUC::convToUCS4<wchar_t>(src);
#else
  wstring c = src;
#endif
  
  GLint		index	= 0;
  GLfloat		X		= 0;
  GLfloat		Y		= 0;
  
  ofPushMatrix();
  ofTranslate(x, y);
  int len = (int)c.length();
  
  while (index < len) {
    int cy = mImpl->getCharID(c[index]);
    if (mImpl->cps[cy].character == mImpl->kTypefaceUnloaded) {
      mImpl->loadChar(cy);
    }
    if (cy < mImpl->limitCharactersNum_) { // full char set or not?
      if (c[index] == L'\n') {
        Y += mImpl->lineHeight_;
        X = 0 ; //reset X Pos back to zero
      }
      else if (c[index] == L' ') {
        int cy = mImpl->getCharID(L'p');
        X += mImpl->cps[cy].width;
        //glTranslated(cps[cy].width, 0, 0);
      }
      else {
        mImpl->drawCharAsShape(cy, X, Y);
        X += mImpl->cps[cy].setWidth;
        //glTranslated(cps[cy].setWidth, 0, 0);
      }
    }
    index++;
  }
  
  ofPopMatrix();
}

void ofxTrueTypeFontUC::drawStringAsShapes(string s, float x, float y) {
  return drawStringAsShapes(util::ofxTrueTypeFontUC::convToWString(s), x, y);
}

//-----------------------------------------------------------
int ofxTrueTypeFontUC::getLoadedCharactersCount() {
  return mImpl->loadedChars.size();
}

//=====================================================================
const int ofxTrueTypeFontUC::Impl::kNumCharcterToStart = 33;  // 0 - 32 are control characters, no graphics needed.
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
    blend_enabled = glIsEnabled(GL_BLEND);
    texture_2d_enabled = glIsEnabled(GL_TEXTURE_2D);
    glGetIntegerv( GL_BLEND_SRC, &blend_src );
    glGetIntegerv( GL_BLEND_DST, &blend_dst );
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
    if (!blend_enabled)
      glDisable(GL_BLEND);
    if  (!texture_2d_enabled)
      glDisable(GL_TEXTURE_2D);
    glBlendFunc( blend_src, blend_dst );
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
  int cy = getCharID(L'p');
  loadChar(cy);
}

//-----------------------------------------------------------
int ofxTrueTypeFontUC::Impl::getCharID(const int &c) {
  int tmp = (int)c - kNumCharcterToStart;
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
  FT_Error err = FT_Load_Glyph( face, FT_Get_Char_Index( face, loadedChars[i] + kNumCharcterToStart ), FT_LOAD_DEFAULT );
  if(err)
    ofLog(OF_LOG_ERROR,"ofxTrueTypeFontUC::loadFont - Error with FT_Load_Glyph %i: FT_Error = %d", loadedChars[i] + kNumCharcterToStart, err);
  
  if (bAntiAliased_ == true)
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
    
  if (bMakeContours_) {
    if (printVectorInfo_)
      printf("\n\ncharacter charID %d: \n", i );
    
    charOutlines[i] = makeContoursForCharacter(face);
    if (simplifyAmt_>0)
      charOutlines[i].simplify(simplifyAmt_);
    charOutlines[i].getTessellation();
  }
  
  // -------------------------
  // info about the character:
  cps[i].character = loadedChars[i];
  cps[i].height = face->glyph->bitmap_top;
  cps[i].width = face->glyph->bitmap.width;
  cps[i].setWidth = face->glyph->advance.x >> 6;
  cps[i].topExtent = face->glyph->bitmap.rows;
  cps[i].leftExtent = face->glyph->bitmap_left;
  
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
  
  textures[i].loadData(atlasPixels.getPixels(), atlasPixels.getWidth(), atlasPixels.getHeight(), GL_LUMINANCE_ALPHA);
}




