#include "testApp.h"

//--------------------------------------------------------------
void testApp::setup(){
  myFont.loadFont("dummyFont.ttf", 64, true, true);
  
  sampleString = "あいうえお";
  
  p1 = ofPoint(100, 200);
  p2 = ofPoint(200, 300);
  p3 = ofPoint(300, 400);
  
  rect1 = myFont.getStringBoundingBox(sampleString, p1.x, p1.y);
  rect2 = myFont.getStringBoundingBox(sampleString, p2.x, p2.y);
  rect3 = myFont.getStringBoundingBox(sampleString, p3.x, p3.y);
  
  characters = myFont.getStringAsPoints(sampleString);
}

//--------------------------------------------------------------
void testApp::update(){
  
}

//--------------------------------------------------------------
void testApp::draw(){
  string fpsStr = "frame rate: " + ofToString(ofGetFrameRate(), 2);
  ofDrawBitmapString(fpsStr, 100, 100);
  
  ofLine(rect1.getTopLeft(), rect1.getTopRight());
  ofLine(rect1.getTopRight(), rect1.getBottomRight());
  ofLine(rect1.getBottomRight(), rect1.getBottomLeft());
  ofLine(rect1.getBottomLeft(), rect1.getTopLeft());
  myFont.drawString(sampleString, p1.x, p1.y);
  
  ofLine(rect2.getTopLeft(), rect2.getTopRight());
  ofLine(rect2.getTopRight(), rect2.getBottomRight());
  ofLine(rect2.getBottomRight(), rect2.getBottomLeft());
  ofLine(rect2.getBottomLeft(), rect2.getTopLeft());
  myFont.drawStringAsShapes(sampleString, p2.x, p2.y);
  
  ofLine(rect3.getTopLeft(), rect3.getTopRight());
  ofLine(rect3.getTopRight(), rect3.getBottomRight());
  ofLine(rect3.getBottomRight(), rect3.getBottomLeft());
  ofLine(rect3.getBottomLeft(), rect3.getTopLeft());
  vector<ofPath>::iterator iter = characters.begin();
  for (; iter != characters.end(); ++iter)
    (*iter).draw(p3.x, p3.y);
}

//--------------------------------------------------------------
void testApp::keyPressed(int key){
  
}

//--------------------------------------------------------------
void testApp::keyReleased(int key){
  
}

//--------------------------------------------------------------
void testApp::mouseMoved(int x, int y ){
  
}

//--------------------------------------------------------------
void testApp::mouseDragged(int x, int y, int button){
  
}

//--------------------------------------------------------------
void testApp::mousePressed(int x, int y, int button){
  
}

//--------------------------------------------------------------
void testApp::mouseReleased(int x, int y, int button){
  
}

//--------------------------------------------------------------
void testApp::windowResized(int w, int h){
  
}

//--------------------------------------------------------------
void testApp::gotMessage(ofMessage msg){
  
}

//--------------------------------------------------------------
void testApp::dragEvent(ofDragInfo dragInfo){
  
}
