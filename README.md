# ofxTrueTypeFontUC

![ofxTrueTypeFontUC Screenshot](http://hironishihara.github.io/ofxTrueTypeFontUC/img/screenshot.png)

## Overview

An extension of ofTrueTypeFont class for using UNICODE characters. Tested on OSX, iOS, and Windows.

## Requirement

No additional library is required.

## Installation

1. Download it ( https://github.com/hironishihara/ofxTrueTypeFontUC/archive/master.zip )
1. Extract the zip file, and rename the extracted folder to ```ofxTrueTypeFontUC```
1. Put the ```ofxTrueTypeFontUC``` folder in the ```addons``` folder in your openFrameworks root folder

## Usage

1. Add ```ofxTrueTypeFontUC.h``` and ```ofxTrueTypeFontUC.cpp``` to your project  
   (Refer to IDE setup guide http://openframeworks.cc/setup/ for detail)
1. Put font file (ex. ```yourFont.ttf```) in ```yourProjectFolder/bin/data``` folder
1. Include ```ofxTrueTypeFontUC.h```

   ```cpp
   // ofApp.h
   #include "ofxTrueTypeFontUC.h"
   ```

1. Declare an instance

   ```cpp
   // ofApp.h
   class ofApp : public ofBaseApp{
     ...
     ofxTrueTypeFontUC myFont;
   };
   ```

1. Load a font

   ```cpp
   // ofApp.cpp
   void ofApp::setup(){
     ...
     myFont.loadFont("yourFont.ttf", 64, true, true);
   }
   ```

1. Draw string

   ```cpp
   // ofApp.cpp
   void ofApp::draw(){
     ...
     myFont.drawStringAsShapes("こんにちは", 100, 100);
     myFont.drawString("はじめまして", 100, 200);
   }
   ```

Refer ```ofxTrueTypeFontUC.h``` with regard to other functions.

## Contribution

1. Fork it ( http://github.com/hironishihara/ofxTrueTypeFontUC/fork )
1. Create your feature branch (git checkout -b my-new-feature)
1. Commit your changes (git commit -am 'Add some feature')
1. Rebase your local changes against the master branch
1. Confirm that your feature works just as you intended
1. Confirm that your feature properly works together with functions of ofxTrueTypeFontUC
1. Push to the branch (git push origin my-new-feature)
1. Create new Pull Request

## Tested System

- OSX (10.10) + Xcode (6.1) + openFrameworks 0.8.4 (osx)
- iOS (8.1) + Xcode (6.1) + openFrameworks 0.8.4 (ios)
- Windows 8.1 + Visual Studio 2012 Express + openFrameworks 0.8.4 (windows)

## License

ofxTrueTypeFontUC is distributed under the MIT License.
This gives everyone the freedoms to use ofxTrueTypeFontUC in any context: commercial or non-commercial, public or private, open or closed source.
Please see License.txt for details.
