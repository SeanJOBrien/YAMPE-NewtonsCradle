#include "ofApp.h"

using namespace YAMPE; using namespace P;


//--------------------------------------------------------------
void ofApp::setup() {
    
    easyCam.setDistance(RANGE);
    
    // instantiate the ground
    ground.set(RANGE, RANGE);
    ground.rotate(90, 1,0,0);
    
    // lift camera to 'eye' level
    cameraHeightRatio = 0.2f;
    float d = easyCam.getDistance();
    easyCam.setPosition(0, cameraHeightRatio*d, d*sqrt(1.0f-cameraHeightRatio*cameraHeightRatio));
    easyCam.setTarget(ofVec3f::zero());
    
    // specify items on scene
    isGridVisible = false;
    isAxisVisible = true;
    isGroundVisible = true;
    
    // create a minimal gui
    gui = new ofxUICanvas();
    
    gui->addWidgetDown(new ofxUILabel("My GUI", OFX_UI_FONT_MEDIUM));
    gui->addSpacer();
    gui->addIntSlider("Falling Balls", 1, 5, &numFalling);
    gui->addIntSlider("Sitting Balls", 1, 20, &numSitting);
    gui->addSlider("Starting Angle", 0, 90, &startAngle);

    
    gui->addSpacer();
    gui->addLabelButton("Reset", false);
    gui->addLabelButton("Quit", false);
    gui->autoSizeToFitWidgets();
    ofAddListener(gui->newGUIEvent, this, &ofApp::guiEvent);
    gui->loadSettings("GUI/guiSettings.xml");
    
    isForceVisible = false;
    
    gravity =ForceGenerator::Ref( new GravityForceGenerator(ofVec3f(0, -1, 0)));
    
    contacts = ContactRegistry::Ref(new ContactRegistry(100, "All contacts"));
    
    reset();
    
}

//--------------------------------------------------------------
void ofApp::update() {
    
    float dt = 1.0f / ofGetFrameRate();
    
    // apply forces on particles
    forceGenerators.applyForce(dt);
    
    // update all particles
    foreach (p, particles) (*p)->integrate(dt);
    foreach (c, constraints) (*c)->generate(contacts);
    
    groundContactGenerator.generate(contacts);
    ppContactGenerator.generate(contacts);
    contacts->resolve(dt);
    contacts->clear();
    
}

//--------------------------------------------------------------
void ofApp::draw() {
    
    ofEnableDepthTest();
    ofBackgroundGradient(ofColor(128), ofColor(0), OF_GRADIENT_BAR);
    
    ofPushStyle();
    easyCam.begin();
    
    if (isGridVisible) {
        ofDrawGrid(10.0f, 5.0f, false, true, true, true);
    } else
        ofDrawGrid(10.0f, 10.0f, false, false, true, false);
    
    if (isAxisVisible) ofDrawAxis(1);
    if (isGroundVisible) {
        ofPushStyle();
        ofSetHexColor(0xB87333);
        ground.draw();
        ofPopStyle();
    }
    
    // render all particles
    
    int in = 0;
    foreach (p, particles)
    {
        (**p).draw();
        ofLine((*p)->position, anchors[in]);
        ++ in ;
    }
    
    easyCam.end();
    
    ofPopStyle();
    
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key) {
    
    float d = easyCam.getDistance();
    switch (key) {
        case 'h':   // hide/show gui
            gui->toggleVisible();
            break;
        case 'a':
            isAxisVisible = !isAxisVisible;
            break;
        case 'd':
            isGridVisible = !isGridVisible;
            break;
        case 'g':
            isGroundVisible = !isGroundVisible;
            break;
        case 'l':   // set the ground to be level
            easyCam.setTarget(ofVec3f::zero());
            break;
        case 'z':
            easyCam.setPosition(0, cameraHeightRatio*d,
                                d*sqrt(1.0f-cameraHeightRatio*cameraHeightRatio));
            easyCam.setTarget(ofVec3f::zero());
            break;
        case 'Z':
            easyCam.setPosition(0, 0, d);
            easyCam.setTarget(ofVec3f::zero());
            break;
        case 'x':
            easyCam.setPosition(d*sqrt(1.0f-cameraHeightRatio*cameraHeightRatio), cameraHeightRatio*d, 0);
            easyCam.setTarget(ofVec3f::zero());
            break;
        case 'X':
            easyCam.setPosition(d, 0, 0);
            easyCam.setTarget(ofVec3f::zero());
            break;
        case 'Y':
            easyCam.setPosition(0.001, d, 0);
            easyCam.setTarget(ofVec3f::zero());
            break;
        case 'f':
            isFullScreen = !isFullScreen;
            if (isFullScreen) {
                ofSetFullscreen(false);
            } else {
                ofSetFullscreen(true);
            }
            break;
            
            // simulation specific keys
        case 'p':
            isForceVisible = !isForceVisible;
            foreach (p, particles) (**p).isForceVisible = isForceVisible;
            break;
        case 'r':
            reset();
            break;
    }
    
}


//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button) {
    
    if (x< gui->getRect()->getWidth() && y < gui->getRect()->getHeight()) return;
    
    easyCam.enableMouseInput();
}


//--------------------------------------------------------------
void ofApp::windowResized(int w, int h) { }

//--------------------------------------------------------------
void ofApp::keyReleased(int key) {}
void ofApp::mouseMoved(int x, int y ) {}
void ofApp::mouseDragged(int x, int y, int button) {}
void ofApp::mouseReleased(int x, int y, int button) {}
void ofApp::gotMessage(ofMessage msg) {}
void ofApp::dragEvent(ofDragInfo dragInfo) {}

//--------------------------------------------------------------
void ofApp::exit() {
    gui->saveSettings("GUI/guiSettings.xml");
    delete gui;
}

//--------------------------------------------------------------
void ofApp::guiEvent(ofxUIEventArgs &e) {
    
    easyCam.disableMouseInput();
    
    string name = e.widget->getName();
    int kind = e.widget->getKind();
    cout << "got event from: " << name << " and kind is " <<endl;
    if (name == "Starting Angle")
    {
        reset();
    } else if (name == "Falling Balls") {
        reset();
    } else if (name == "Sitting Balls") {
        reset();
    } else if (name=="Reset") {
        reset();
    } else if (name=="Quit") {
        ofExit();
    }
}

//--------------------------------------------------------------
void ofApp::reset() {
    forceGenerators.clear();
    particles.clear();
    constraints.clear();
    anchors.clear();
    ppContactGenerator.particles.clear();
    groundContactGenerator.particles.clear();
    int totalBalls = numFalling+numSitting;
    float offset = (totalBalls/2-1)*0.6;
    for(int i = 0; i < totalBalls;i++)
    {
        Particle::Ref p = Particle::Ref(new Particle());
        p->setLabel(MAKE_STRING("Ball 2"));
        (i < numFalling)? p->bodyColor = ofColor (255, 0, 0):p->bodyColor = ofColor (0, 0, 0);
        p->wireColor = p->bodyColor;
        p->radius = 0.3;
        p->setPosition(ofVec3f((i*0.6)-0.3-offset,0.3,0.0));
        
        //-(i*0.6)-0.3
        //Anchor Sitting (worked out from loop
        ofVec3f anchorPoint = p->position + ofVec3f(0,5.0,0);
        
        //Anchor particle
        EqualityAnchoredConstraint::Ref anchor = EqualityAnchoredConstraint::Ref(new EqualityAnchoredConstraint(p, anchorPoint,5.0));
        
        
        //SetPosition based on angle and
        if(i < numFalling)
        {
            float length = ((anchorPoint-p->position).length());
            p->setPosition(ofVec3f(anchorPoint.x +(length * -sin(startAngle * pi / 180)), anchorPoint.y + (length * -cos(startAngle* pi / 180)), 0.0));
        }
        
        //Store All
        anchors.push_back(anchorPoint);
        forceGenerators.add(p, gravity);
        constraints.push_back(anchor);
        particles.push_back(p);
        groundContactGenerator.particles.push_back(p);
        ppContactGenerator.particles.push_back(p);
    }
}
