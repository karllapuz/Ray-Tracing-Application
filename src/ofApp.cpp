#include "ofApp.h"

// Intersect Ray with Plane  (wrapper on glm::intersect*
//
bool Plane::intersect(const Ray &ray, glm::vec3 & point, glm::vec3 & normalAtIntersect) {
    float dist;
    bool hit = glm::intersectRayPlane(ray.p, ray.d, position, this->normal, dist);
    if (hit) {
        Ray r = ray;
        point = r.evalPoint(dist);
        normalAtIntersect = this->normal;
    }
    return (hit);
}


// Convert (u, v) to (x, y, z)
// We assume u,v is in [0, 1]
//
glm::vec3 ViewPlane::toWorld(float u, float v) {
    float w = width();
    float h = height();
    return (glm::vec3((u * w) + min.x, (v * h) + min.y, position.z));
}

// Get a ray from the current camera position to the (u, v) position on
// the ViewPlane
//
Ray RenderCam::getRay(float u, float v) {
    glm::vec3 pointOnPlane = view.toWorld(u, v);
    return(Ray(position, glm::normalize(pointOnPlane - position)));
}

// This could be drawn a lot simpler but I wanted to use the getRay call
// to test it at the corners.
//
void RenderCam::drawFrustum() {
    view.draw();
    Ray r1 = getRay(0, 0);
    Ray r2 = getRay(0, 1);
    Ray r3 = getRay(1, 1);
    Ray r4 = getRay(1, 0);
    float dist = glm::length((view.toWorld(0, 0) - position));
    r1.draw(dist);
    r2.draw(dist);
    r3.draw(dist);
    r4.draw(dist);
}

//--------------------------------------------------------------
void ofApp::setup(){
    
    // GUI Setup
    gui.setup();
    gui.add(ambientPercent.setup("Ambient Percentage: ", 0.3, 0, 1));
    gui.add(lightIntensity.setup("Light Intensity: ", 1, 0, 5));
    gui.add(phongExponent.setup("Phong Exponent: ", 10, 10, 1000));
    
    ofSetBackgroundColor(ofColor::black);
    mainCam.setDistance(30);
    mainCam.setNearClip(.1);
    sideCam.setPosition(40, 0, 0);
    sideCam.lookAt(glm::vec3(0, 0, 0));
    previewCam.setPosition(glm::vec3(0, 0, 10));
    theCam = &mainCam;
    
    Light light1 = Light(glm::vec3(4, 0.5, 1), lightIntensity, ofColor::white);
    Light light2 = Light(glm::vec3(-2, 4, 4), lightIntensity, ofColor::white);
    
    scene.push_back(new Sphere(glm::vec3(-1, 0, 0), 2.0, ofColor::blue));
    scene.push_back(new Sphere(glm::vec3(1, 0, -4), 2.0, ofColor::lightGreen));
    scene.push_back(new Sphere(glm::vec3(0, 0, 2), 1.0, ofColor::red));
    scene.push_back(new Plane(glm::vec3(0, -2, 0), glm::vec3(0, 1, 0), ofColor::gray));
    scene.push_back(new Light(glm::vec3(4, 0.5, 1), lightIntensity, ofColor::white));
    scene.push_back(new Light(glm::vec3(-2, 4, 4), lightIntensity, ofColor::white));
    
    lights.push_back(light1);
    lights.push_back(light2);
}

//--------------------------------------------------------------
void ofApp::update(){
    
}

//--------------------------------------------------------------
void ofApp::draw(){
    
    gui.draw();
    
    theCam->begin();
    
    ofNoFill();
    
    for (int i = 0; i < scene.size(); i++) {
        SceneObject * obj = scene[i];
        ofSetColor(obj->diffuseColor);
        obj->draw();
    }
    
    ofSetColor(ofColor::lightSkyBlue);
    renderCam.drawFrustum();
    
    ofSetColor(ofColor::white);
    renderCam.draw();
    // rayTrace();
    
    if (bShowGrid) { drawGrid(); }
    if (bShowImage) {
        image.draw(renderCam.view.toWorld(0, 0).x, renderCam.view.toWorld(0, 0).y, renderCam.view.toWorld(0, 0).z, renderCam.view.width(), renderCam.view.height());
    }
    
    theCam->end();
    
}

void ofApp::rayTrace(){
    // Set width and height of the image based on the aspect ratio
    image.allocate(imageWidth, imageHeight, OF_IMAGE_COLOR);
    
    for (float i = 0; i < image.getWidth(); i++) {
        for (float j = 0; j < image.getHeight(); j++) {
            float u = (i + 0.5f) / image.getWidth();
            float v = (image.getHeight() - j - 1 + 0.5f) / image.getHeight();
            Ray currentRay = renderCam.getRay(u, v);
            
            float closestObjDistance = std::numeric_limits<float>::infinity();
            ofColor colorToDraw = ofColor::black; // default black for when it does not hit
            // currentRay.draw(150);
            glm::vec3 pt, normal;
            for (int k = 0; k < scene.size() - lights.size(); k++) {
                SceneObject* obj = scene[k];
                bool hit = obj->intersect(currentRay, pt, normal);
                if (hit) {
                    float distance = glm::length(pt - renderCam.position);
                    if (distance < closestObjDistance) {
                        closestObjDistance = distance;
                        colorToDraw = scene[k]->diffuseColor;
                        colorToDraw = ambient(colorToDraw, ambientPercent) + lambert(pt, normal, colorToDraw) + phong(pt, normal, colorToDraw, scene[k]->specularColor, phongExponent);
                    }
                }
            }
            image.setColor(i, j, colorToDraw);
        }
    }
    image.update();
    image.save("RayTracing.jpg", OF_IMAGE_QUALITY_HIGH);
    
}

void ofApp::drawGrid() {
    for (int x = 1; x < image.getWidth(); x++) {
        glm::vec3 firstPoint = renderCam.view.toWorld(x / image.getWidth(), renderCam.view.max.y);
        glm::vec3 secondPoint = renderCam.view.toWorld(x / image.getWidth(), renderCam.view.min.y);
        ofDrawLine(firstPoint, secondPoint);
    }
    for (int y = 1; y < image.getHeight(); y++) {
        glm::vec3 firstPoint = renderCam.view.toWorld(renderCam.view.min.x, y / image.getHeight());
        glm::vec3 secondPoint = renderCam.view.toWorld(renderCam.view.max.x, y / image.getHeight());
        ofDrawLine(firstPoint, secondPoint);
    }
}

ofColor ofApp::ambient(const ofColor diffuse, float percentage) {
    return diffuse * percentage;
}

ofColor ofApp::lambert(const glm::vec3 &p, const glm::vec3 &norm, const ofColor diffuse) {
    ofColor diffusedColor = ofColor::black;
    for (int i = 0; i < lights.size(); i++) {
        Light light = lights[i];
        diffusedColor += max(float(0.0), glm::dot(norm, glm::normalize(light.position - p))) * lightIntensity * diffuse;
    }
    return diffusedColor;
}

ofColor ofApp::phong(const glm::vec3 &p, const glm::vec3 &norm, const ofColor diffuse, const ofColor specular, float power) {
    ofColor diffusedColor = ofColor::black;
    for (int i = 0; i < lights.size(); i++) {
        Light light = lights[i];
        glm::vec3 v = glm::normalize(renderCam.position - p);
        glm::vec3 l = glm::normalize(light.position - p);
        // Solve for the bisector
        glm::vec3 b = (v + l) / glm::length(v + l);
        diffusedColor += specular * lightIntensity * glm::pow(glm::dot(norm, b), power);
    }
    return diffusedColor;
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){
    
}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){
    switch (key) {
        case 'C':
        case 'c':
            if (mainCam.getMouseInputEnabled()) mainCam.disableMouseInput();
            else mainCam.enableMouseInput();
            break;
        case 'F':
        case 'b':
            break;
        case 'f':
            ofToggleFullscreen();
            break;
        case 'h':
            bHide = !bHide;
            break;
        case '1':
            theCam = &mainCam;
            break;
        case '2':
            theCam = &sideCam;
            break;
        case '3':
            theCam = &previewCam;
            break;
        case 'G':
        case 'g':
            bShowGrid = !bShowGrid;
            break;
        case 'I':
        case 'i':
            bShowImage = !bShowImage;
            break;
        case 'R':
        case 'r':
            rayTrace();
            break;
    }
}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){
    
}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){
    
}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){
    
}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){
    
}

//--------------------------------------------------------------
void ofApp::mouseEntered(int x, int y){
    
}

//--------------------------------------------------------------
void ofApp::mouseExited(int x, int y){
    
}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){
    
}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){
    
}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){
    
}
