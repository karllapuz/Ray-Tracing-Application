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
    gui.add(ambientPercent.setup("Ambient Percentage: ", 0.1, 0, 1));
    gui.add(lightIntensity.setup("Light Intensity: ", 0.80, 0, 5));
    gui.add(phongExponent.setup("Phong Exponent: ", 10, 10, 1000));
    
    ofSetBackgroundColor(ofColor::black);
    mainCam.setDistance(30);
    mainCam.setNearClip(.1);
    sideCam.setPosition(40, 0, 0);
    sideCam.lookAt(glm::vec3(0, 0, 0));
    previewCam.setPosition(glm::vec3(0, 0, 10));
    theCam = &mainCam;
    
    Light * light1 = new Light(glm::vec3(4, 6, 4), lightIntensity, ofColor::white);
    Light * light2 = new Light(glm::vec3(-4, 6, 4), lightIntensity, ofColor::white);
    
    scene.push_back(new Sphere(glm::vec3(-1, 0, 0), 2.0, ofColor::blue));
    scene.push_back(new Sphere(glm::vec3(1, 0, -4), 2.0, ofColor::lightGreen));
    scene.push_back(new Sphere(glm::vec3(0, 0, 2), 1.0, ofColor::red));
    scene.push_back(new Plane(glm::vec3(0, -2, 0), glm::vec3(0, 1, 0), ofColor::gray));
    scene.push_back(light1);
    scene.push_back(light2);
    
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
        if (objSelected() && obj == selected[0]) ofSetColor(ofColor::white);
        else ofSetColor(obj->diffuseColor);
        obj->draw();
    }
    
    ofSetColor(ofColor::lightSkyBlue);
    renderCam.drawFrustum();
    
    ofSetColor(ofColor::white);
    renderCam.draw();
    // render();
    
    if (bShowGrid) { drawGrid(); }
    if (bShowImage) {
        image.draw(renderCam.view.toWorld(0, 0).x, renderCam.view.toWorld(0, 0).y, renderCam.view.toWorld(0, 0).z, renderCam.view.width(), renderCam.view.height());
    }
    
    theCam->end();
    
}

void ofApp::render(){
    // Set width and height of the image based on the aspect ratio
    image.allocate(imageWidth, imageHeight, OF_IMAGE_COLOR);
    
    for (float i = 0; i < image.getWidth(); i++) {
        for (float j = 0; j < image.getHeight(); j++) {
            float iNudge = i + 0.5f;
            float jNudge = j + 0.5f;
            
            float u = iNudge / image.getWidth();
            float v = (image.getHeight() - jNudge) / image.getHeight();
            
            // Use a 4x4 grid for anti aliasing
            int nSquares = 2;
            ofColor colorSum = ofColor::black;
            for (float x = -(nSquares - 1.0f) / 2.0f; x <= (nSquares - 1.0f) / 2.0f; x++) {
                for (float y = -(nSquares - 1.0f) / 2.0f; y <= (nSquares - 1.0f) / 2.0f; y++) {
                    // cout << u << x << image.getWidth() << endl;
                    float uTemp = u + (x / (image.getWidth() * nSquares));
                    float vTemp = v + (y / (image.getHeight() * nSquares));
                    // cout << "(" << uTemp << ", " << vTemp << ")" << endl;
                    Ray currentRay = renderCam.getRay(uTemp, vTemp);
                    // currentRay.draw(150);
                    colorSum += (rayTrace(currentRay) / (nSquares * nSquares));
                }
            }
            // colorSum = colorSum / (nSquares * nSquares);
            image.setColor(i, j, colorSum);
            
            
//            Ray currentRay = renderCam.getRay(u, v);
//            // currentRay.draw(150);
//            ofColor colorToDraw = rayTrace(currentRay); // default black for when it does not hit
//            image.setColor(i, j, colorToDraw);
        }
    }
    image.update();
    image.save("RayTracing.jpg", OF_IMAGE_QUALITY_HIGH);
    
}

ofColor ofApp::rayTrace(const Ray &ray) {
    ofColor colorToDraw = ofColor::black; // default black for when it does not hit
    float closestObjDistance = std::numeric_limits<float>::infinity();
    glm::vec3 pt, normal;
    for (int i = 0; i < scene.size() - lights.size(); i++) {
        SceneObject* obj = scene[i];
        bool hit = obj->intersect(ray, pt, normal);
        if (hit) {
            float distance = glm::length(pt - renderCam.position);
            if (distance < closestObjDistance) {
                closestObjDistance = distance;
                colorToDraw = scene[i]->diffuseColor;
                colorToDraw = ambient(colorToDraw, ambientPercent) + phong(pt, normal, colorToDraw, scene[i]->specularColor, phongExponent);
            }
        }
    }
    return colorToDraw;
}

bool ofApp::inShadow (const Ray &ray) {
    glm::vec3 pt, normal;
    for (int i = 0; i < scene.size() - lights.size(); i++) {
        if (scene[i]->intersect(ray, pt, normal)) return true;
    }
    return false;
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

ofColor ofApp::phong(const glm::vec3 &p, const glm::vec3 &norm, const ofColor diffuse, const ofColor specular, float power) {
    ofColor diffusedColor = ofColor::black;
    for (int i = 0; i < lights.size(); i++) {
        Light * light = lights[i];
        glm::vec3 v = glm::normalize(renderCam.position - p);
        glm::vec3 l = glm::normalize(light->position - p);
        float epsilon = 0.001;
        glm::vec3 epsilonDistance = p + epsilon * l;
        Ray lightRay = Ray(epsilonDistance, l);

        // Check whether the point is in shadow or not
        if (!inShadow(lightRay)) {
            // Solve for the bisector
            glm::vec3 b = (v + l) / glm::length(v + l);
            ofColor lambert = max(float(0.0), glm::dot(norm, l)) * lightIntensity * diffuse;
            ofColor phong = specular * lightIntensity * glm::pow(glm::dot(norm, b), power);
            diffusedColor += phong + lambert;
        }
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
            if (mainCam.getMouseInputEnabled()) {
                selected.clear();
                mainCam.disableMouseInput();
            }
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
        case 'x':
            rotateX = true;
            break;
        case 'y':
            rotateY = true;
            break;
        case 'z':
            rotateZ = true;
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
            render();
            break;
    }
}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){
    
}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){
    
    if (objSelected() && bDrag) {
        glm::vec3 point;
        mouseToDragPlane(x, y, point);
        if (rotateX) {
            selected[0]->rotation += glm::vec3((point.x - lastPoint.x) * 20.0, 0, 0);
        }
        else if (rotateY) {
            selected[0]->rotation += glm::vec3(0, (point.x - lastPoint.x) * 20.0, 0);
        }
        else if (rotateZ) {
            selected[0]->rotation += glm::vec3(0, 0, (point.x - lastPoint.x) * 20.0);
        }
        else {
            selected[0]->position += (point - lastPoint);
        }
        lastPoint = point;
    }
    
}

bool ofApp::mouseToDragPlane(int x, int y, glm::vec3 &point) {
    glm::vec3 p = theCam->screenToWorld(glm::vec3(x, y, 0));
    glm::vec3 d = p - theCam->getPosition();
    glm::vec3 dn = glm::normalize(d);
    
    float dist;
    glm::vec3 pos;
    if (objSelected()) {
        pos = selected[0]->position;
    }
    else pos = glm::vec3(0, 0, 0);
    if (glm::intersectRayPlane(p, dn, pos, glm::normalize(theCam->getZAxis()), dist)) {
        point = p + dn * dist;
        return true;
    }
    return false;
}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){
    
    // if we are moving the camera around, don't allow selection
    //
    if (mainCam.getMouseInputEnabled()) return;
    
    // clear selection list
    //
    selected.clear();
    
    //
    // test if something selected
    //
    vector<SceneObject *> hits;
    
    glm::vec3 p = theCam->screenToWorld(glm::vec3(x, y, 0));
    glm::vec3 d = p - theCam->getPosition();
    glm::vec3 dn = glm::normalize(d);
    
    // check for selection of scene objects
    //
    for (int i = 0; i < scene.size(); i++) {
        
        glm::vec3 point, norm;
        
        //  We hit an object
        //
        if (scene[i]->intersect(Ray(p, dn), point, norm) && scene[i]->isSelectable) {
            hits.push_back(scene[i]);
        }
    }
    
    
    // if we selected more than one, pick nearest
    //
    SceneObject *selectedObj = NULL;
    if (hits.size() > 0) {
        selectedObj = hits[0];
        float nearestDist = std::numeric_limits<float>::infinity();
        for (int n = 0; n < hits.size(); n++) {
            float dist = glm::length(hits[n]->position - theCam->getPosition());
            if (dist < nearestDist) {
                nearestDist = dist;
                selectedObj = hits[n];
            }
        }
    }
    if (selectedObj) {
        selected.push_back(selectedObj);
        bDrag = true;
        mouseToDragPlane(x, y, lastPoint);
    }
    else {
        selected.clear();
    }
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
