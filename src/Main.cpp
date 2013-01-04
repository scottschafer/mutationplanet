/************************************************************************
 MutationPlanet
 Copyright (C) 2012, Scott Schafer
 
 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
 ************************************************************************/


/**
 Main
 
 Handles initialization, UI and rendering
 **/

#include "Main.h"
#include "SphereWorld.h"
#include "InstructionSet.h"
#include "Agent.h"
#include "UtilsRandom.h"
#include <pthread.h>
#include "Parameters.h"


// Declare our game instance
Main game;
static SphereWorld world;

int numTurns = 0;
float totalElapsedTime = 0;

Main::Main()
: _scene(NULL)
{
    mViewScale = 1;
    
    memset(mSegmentBatch, 0, sizeof(mSegmentBatch));
    for (int i = 0; i < sizeof(world.mAgents)/sizeof(world.mAgents[0]); i++)
    {
        Agent * pAgent = &world.mAgents[i];
        pAgent->mStatus = eNonExistent;
        for (int j = 0; j < sizeof(pAgent->mSegments)/sizeof(pAgent->mSegments[0]); j++)
        {
            SphereEntity * pEntity = &pAgent->mSegments[j];
            pEntity->mAgent = pAgent;
        }
    }
}

inline Vector3 getRandomSpherePoint()
{
    Vector3 v(UtilsRandom::getUnitRandom(), UtilsRandom::getUnitRandom(), UtilsRandom::getUnitRandom());
    v.normalize();
    
    return v;
}

pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;
int  counter = 0;

bool threadAlive = false;

void * Main :: threadFunction(void*)
{
    threadAlive = true;
    while (threadAlive)
    {
        ++numTurns;
        if (Parameters::speed == 0)
        {
            // stopped
            usleep(100);
        }
        else
        {
            pthread_mutex_lock( &mutex1 );
            world.step();
            pthread_mutex_unlock( &mutex1 );
            uint sleepTime = 10 * (10 - Parameters::speed);
            sleepTime *= sleepTime * sleepTime;
            
            if (sleepTime)
                usleep(sleepTime);
        }
    }
    return NULL;
}

#define LABEL_WIDTH 120
#define SLIDER_WIDTH 100

void Main :: createControlHeader(Form *form, std::string text)
{
    Theme::Style * pNoBorder = form->getTheme()->getStyle("noBorder");
    
    Container *pContainer = Container::create("", pNoBorder);
    Container *pMainContainer = (Container*)form->getControl("main");
    float height = pMainContainer->getControls().size() ? 40 : 25;
    pContainer->setSize(300, height);
    pContainer->setZIndex(pMainContainer->getControls().size());
    pMainContainer->addControl(pContainer);
    
    Label *pLabel = Label::create("", pNoBorder);
    pContainer->addControl(pLabel);
    pLabel->setTextAlignment(Font::ALIGN_BOTTOM_LEFT);
    pLabel->setText(text.c_str());
    pLabel->setSize(300, height);
}

void Main :: createSpacer(Form *form, float height)
{
    Theme::Style * pNoBorder = form->getTheme()->getStyle("noBorder");
    
    Container *pContainer = Container::create("", pNoBorder);
    Container *pMainContainer = (Container*)form->getControl("main");
    pContainer->setSize(300, height);
    pContainer->setZIndex(pMainContainer->getControls().size());
    pMainContainer->addControl(pContainer);
}

Slider * Main :: createSliderControl(Form *form, std::string id, std::string label, float minValue, float maxValue, float step /*= 0 */)
{
    Theme::Style * pNoBorder = form->getTheme()->getStyle("noBorder");
    
    Container *pContainer = Container::create("", pNoBorder);
    Container *pMainContainer = (Container*)form->getControl("main");
    pContainer->setZIndex(pMainContainer->getControls().size());
    pMainContainer->addControl(pContainer);
    pContainer->setSize(300, 30);
    
    Label *pLabel = Label::create("", pNoBorder);
    pContainer->addControl(pLabel);
    pLabel->setTextAlignment(Font::ALIGN_BOTTOM_LEFT);
    pLabel->setText(label.c_str());
    pLabel->setSize(LABEL_WIDTH, 30);
    pLabel->setPosition(5, 0);
    
    Slider *pSlider = Slider::create(id.c_str(), pNoBorder);
    pContainer->addControl(pSlider);
    pSlider->setSize(SLIDER_WIDTH, 30);
    pSlider->setPosition(LABEL_WIDTH,0);
    pSlider->setMin(minValue);
    pSlider->setMax(maxValue);
    pSlider->setStep(step);
    
    Label *pValueLabel = Label::create((id + "Label").c_str(), pNoBorder);
    pContainer->addControl(pValueLabel);
    pValueLabel->setTextAlignment(Font::ALIGN_BOTTOM_LEFT);
    pValueLabel->setSize(60, 30);
    pValueLabel->setPosition(SLIDER_WIDTH + LABEL_WIDTH, 0);
    
    pSlider->addListener(this, Listener::VALUE_CHANGED);
    
    return pSlider;
}

CheckBox * Main :: createCheckboxControl(Form *form, std::string label)
{
    Theme::Style * pNoBorder = form->getTheme()->getStyle("noBorder");
    
    Container *pContainer = Container::create("", pNoBorder);
    Container *pMainContainer = (Container*)form->getControl("main");
    float height = 30;
    pContainer->setSize(300, height);
    pContainer->setZIndex(pMainContainer->getControls().size());
    pMainContainer->addControl(pContainer);
    
    CheckBox *pCheckbox = CheckBox::create("", pNoBorder);
    pCheckbox->setTextAlignment(Font::ALIGN_BOTTOM_LEFT);
    pContainer->addControl(pCheckbox);
    pCheckbox->setText(label.c_str());
    pCheckbox->setSize(300, height);
    pCheckbox->setImageSize(20, 20);
    
    pCheckbox->addListener(this, Listener::VALUE_CHANGED);
    
    return pCheckbox;
}

Button * Main :: createButton(Form *form, std::string label)
{
    Theme::Style * pNoBorder = form->getTheme()->getStyle("noBorder");
    Theme::Style * pBasic = form->getTheme()->getStyle("basic");
    
    Container *pContainer = Container::create("", pNoBorder);
    Container *pMainContainer = (Container*)form->getControl("main");
    float height = 40;
    pContainer->setSize(300, height);
    pContainer->setZIndex(pMainContainer->getControls().size());
    pMainContainer->addControl(pContainer);
    
    Button *pButton = Button::create("", pBasic);
    pContainer->addControl(pButton);
    pButton->setText(label.c_str());
    pButton->setSize(280, height);
    
    pButton->addListener(this, Listener::CLICK);
    return pButton;
}



void Main::initialize()
{
    registerGesture(Gesture::GestureEvent(Gesture::GESTURE_PINCH));
    
    mViewRotateMatrix.identity();
    // Load game scene from file
    Bundle* bundle = Bundle::create("res/box.gpb");
    _scene = bundle->loadScene();
    SAFE_RELEASE(bundle);
    
    // Create a font for drawing the framerate.
//    _font = Font::create("res/arial18.gpb");
    _font = Font::create("res/modata18.gpb");
//    _font = Font::create("res/arial40.gpb");
    
    createUI();
    
    // Store pointers to UI controls we care about.
    
    // Set the aspect ratio for the scene's camera to match the current resolution
    _scene->getActiveCamera()->setAspectRatio((float)getWidth() / (float)getHeight());
    
    // Get light node
    Node* lightNode = _scene->findNode("directionalLight");
    Light* light = lightNode->getLight();
    
    // Initialize box model
    Node* boxNode = _scene->findNode("box");
    Model* boxModel = boxNode->getModel();
    Material* boxMaterial = boxModel->setMaterial("res/box.material");
    boxMaterial->getParameter("u_ambientColor")->setValue(_scene->getAmbientColor());
    boxMaterial->getParameter("u_lightColor")->setValue(light->getColor());
    boxMaterial->getParameter("u_lightDirection")->setValue(lightNode->getForwardVectorView());
    
    InstructionSet::reset();
    SpriteBatch * segmentDefault = SpriteBatch::create("res/segment.png");
    for (int i = 0; i < 255; i++)
        mSegmentBatch[i] = segmentDefault;
    
    mSegmentBatch[0] = SpriteBatch::create("res/sphere.png");
    mSegmentBatch[1] = SpriteBatch::create("res/ActiveSegment.png");
    mSegmentBatch[2] = SpriteBatch::create("res/segmentFrame.png");
    mSegmentBatch[eBarrier1] = mSegmentBatch[eBarrier2] = mSegmentBatch[eBarrier3] = SpriteBatch::create("res/barrier.png");
    
    mSegmentBatch[eInstructionMoveAndEat] = SpriteBatch::create("res/segment_M.png");
    mSegmentBatch[eInstructionMove] = SpriteBatch::create("res/segment_n.png");
    mSegmentBatch[eInstructionTurnLeft] = SpriteBatch::create("res/segment_<.png");
    mSegmentBatch[eInstructionTurnRight] = SpriteBatch::create("res/segment_>.png");
    mSegmentBatch[eInstructionHardTurnLeft] = SpriteBatch::create("res/segment_[.png");
    mSegmentBatch[eInstructionHardTurnRight] = SpriteBatch::create("res/segment_].png");
    mSegmentBatch[eInstructionSleep] = SpriteBatch::create("res/segment_Z.png");
    
    mSegmentBatch[eInstructionTestSeeFood] = SpriteBatch::create("res/segment_test_see_food.png");
    mSegmentBatch[eInstructionTestNotSeeFood] = SpriteBatch::create("res/segment_test_not_see_food.png");
    mSegmentBatch[eInstructionTestBlocked] = SpriteBatch::create("res/segment_test_blocked.png");
    mSegmentBatch[eInstructionTestNotBlocked] = SpriteBatch::create("res/segment_test_not_blocked.png");

    mSegmentBatch[eInstructionPhotosynthesize] = SpriteBatch::create("res/food.png");
    mSegmentBatch[eInstructionFakePhotosynthesize] = SpriteBatch::create("res/segment_fakefood.png");

    reset();
    
    int rc1;
    
    if( (rc1=pthread_create( &mThread, NULL, &Main::threadFunction, NULL)) )
    {
        printf("Thread creation failed: %d\n", rc1);
    }
}

void Main::createUI()
{
    // Create main form
    _formMain = Form::create("res/editor.form");
    Container *pMainContainer = (Container*)_formMain->getControl("main");
    _formMain->setSize(_formMain->getWidth(), 290);
    pMainContainer->setSize(pMainContainer->getWidth(), 270);
    
    _cellSizeSlider = createSliderControl(_formMain, "cellSize", "Cell size:", 1, 10, 1);
    _speedSlider = createSliderControl(_formMain, "speed", "Speed:", 0, 10, 1);
    _mutationSlider = createSliderControl(_formMain, "mutation", "Mutation:", 0, 25);
    createSpacer(_formMain, 15);
    _barriers1 = createCheckboxControl(_formMain, "Barriers #1");
    _barriers2 = createCheckboxControl(_formMain, "Barriers #2");
    
    _showAdvanced = createCheckboxControl(_formMain, "Show advanced settings");
    
    createSpacer(_formMain, 15);
    _resetButton = createButton(_formMain, "Reset world");
    
    // Create advanced form, optionally shown
    _formAdvanced = Form::create("res/editor.form");
    _formAdvanced->setPosition(_formAdvanced->getX(), 380);
    _formAdvanced->setSize(_formAdvanced->getWidth(), 360);
    Container *pAdvancedContainer = (Container*)_formAdvanced->getControl("main");
    pAdvancedContainer->setSize(pAdvancedContainer->getWidth(), 340);

    createControlHeader(_formAdvanced, "Energy Cost / Gain");
    // _cycleEnergyCostSlider = createSliderControl(_formAdvanced, "cycleEnergyCost", "Cycle:", .5, 2);
    _photoSynthesizeEnergyGainSlider = createSliderControl(_formAdvanced, "photoSynthesizeEnergyGain", "Photosynthesis:", 1, 10);
    _moveEnergyCostSlider = createSliderControl(_formAdvanced, "moveEnergyCost", "Move:", 0, 15);
    _moveAndEatEnergyCostSlider = createSliderControl(_formAdvanced, "moveAndEatEnergyCost", "Move & eat:", 0, 15);
    
    createControlHeader(_formAdvanced,"Spawning");
    _baseSpawnEnergySlider = createSliderControl(_formAdvanced,"baseSpawnEnergy", "Base:", 50, 500);
    _extraSpawnEnergyPerSegmentSlider = createSliderControl(_formAdvanced,"extraSpawnEnergyPerSegment", "Per cell:", 50, 500);
    
    createControlHeader(_formAdvanced,"Other");
    _lookDistanceSlider = createSliderControl(_formAdvanced,"lookDistance", "Vision range:", 1, 15);
    _allowSelfOverlap = createCheckboxControl(_formAdvanced, "Allow self overlap");
    
    _formMain->setConsumeInputEvents(false);
    _formAdvanced->setConsumeInputEvents(false);
    
    _formHelp = Form::create("res/editor.form");
    _formHelp->setPosition(720, 700);
    Theme::Style * pNoBorder = _formHelp->getTheme()->getStyle("noBorder");
    _formHelp->setStyle(pNoBorder);
    _formHelp->getControl("main")->setStyle(pNoBorder);
    _helpButton = createButton(_formHelp, "What is this?");

    setControlValues();
    updateControlLabels();
}

void Main::reset()
{
    pthread_mutex_lock( &mutex1 );
    
    
    for (int i = 0; i < world.mMaxLiveAgentIndex; i++)
    {
        Agent & agent = world.mAgents[i];
        if (agent.mStatus == eAlive)
            world.killAgent(i);
    }
    
    string genome;
    bool bAllowMutation = true;
    int initialCount = 500;
    
    genome += eInstructionPhotosynthesize;
 
    for (int i = 0; i < 500; i++)
    {
        Agent *pAgent = world.createEmptyAgent();
        pAgent->initialize(getRandomSpherePoint(), genome.c_str(), bAllowMutation);
        world.addAgentToWorld(pAgent);
    }
    pthread_mutex_unlock( &mutex1 );
}

static std::string formatText(const char *pFormat, ...)
{
    char buffer[1024];
    va_list argptr;
    va_start(argptr, pFormat);
    vsprintf(buffer, pFormat, argptr);
    va_end(argptr);
    return string(buffer);
}

void Main::controlEvent(Control* control, EventType evt)
{
    switch(evt)
    {
        default:
            break;
            
        case Listener::CLICK:
            if (control == _resetButton)
                reset();
            else if (control == _helpButton)
                printf("help");
            break;
            
        case Listener::VALUE_CHANGED:
            if (control == _speedSlider)
                Parameters::speed = _speedSlider->getValue();
            else if (control == _mutationSlider)
                Parameters::mutationPercent = _mutationSlider->getValue();
            else if (control == _cellSizeSlider)
                Parameters::cellSize = _cellSizeSlider->getValue();
            else if (control == _cycleEnergyCostSlider)
                Parameters::cycleEnergyCost = _cycleEnergyCostSlider->getValue();
            else if (control == _photoSynthesizeEnergyGainSlider)
                Parameters::photoSynthesizeEnergyGain = _photoSynthesizeEnergyGainSlider->getValue();
            else if (control == _moveEnergyCostSlider)
                Parameters::moveEnergyCost = _moveEnergyCostSlider->getValue();
            else if (control == _moveAndEatEnergyCostSlider)
                Parameters::moveAndEatEnergyCost = _moveAndEatEnergyCostSlider->getValue();
            else if (control == _baseSpawnEnergySlider)
                Parameters::baseSpawnEnergy = _baseSpawnEnergySlider->getValue();
            else if (control == _extraSpawnEnergyPerSegmentSlider)
                Parameters::extraSpawnEnergyPerSegment = _extraSpawnEnergyPerSegmentSlider->getValue();
            else if (control == _lookDistanceSlider)
                Parameters::lookDistance = _lookDistanceSlider->getValue();
            else if (control == _allowSelfOverlap)
                Parameters::allowSelfOverlap = _allowSelfOverlap->isChecked();
            else if (control == _barriers1)
                setBarriers(0, _barriers1->isChecked());
            else if (control == _barriers2)
                setBarriers(1, _barriers2->isChecked());
            else if (control == _barriers3)
                setBarriers(2, _barriers3->isChecked());
            updateControlLabels();
            break;
    }
}

void Main :: setBarriers(int type, bool bOn)
{
    pthread_mutex_lock( &mutex1 );
    static char barrierTypes[3] = { eBarrier1, eBarrier2, eBarrier3 };
    char barrierType = barrierTypes[type];
    
    if (! bOn)
    {
        // remove specified barrier type
        for (int i = 0; i < world.mMaxLiveAgentIndex; i++)
        {
            Agent & agent = world.mAgents[i];
            if ((agent.mStatus == eInanimate) && (agent.mSegments[0].mType == barrierType))
                world.killAgent(i);
        }
    }
    
    if (bOn)
    {
        std::string genome;
        genome += barrierType;
        if (type == 0)
        {
            float barrierDistance = .008;
            for (float a = -MATH_PI * .95; a < MATH_PI * .95; a += barrierDistance)
            {
                Agent *pAgent = world.createEmptyAgent(true);
                
                Vector3 v(cos(a),sin(a*20)/10,sin(a));
                v.normalize();
                pAgent->initialize(v, genome.c_str(), true);
                world.addAgentToWorld(pAgent);
                pAgent->mStatus = eInanimate;
                pAgent->mEnergy = pAgent->getSpawnEnergy();
            }
        }
        
        if (type == 1)
        {
            float barrierDistance = .05;
            for (float a = -MATH_PI * .98; a < MATH_PI * .98; a += barrierDistance)
            {
                for (int i = 0; i < 6; i++)
                {
                    Agent *pAgent = world.createEmptyAgent(true);
                    Vector3 v;
                    
                    float size = .5;
                    switch (i)
                    
                    {
                        case 0:
                            v = Vector3(1,cos(a) * size,sin(a) * size);
                            break;
                        case 1:
                            v = Vector3(-1,cos(a) * size,sin(a) * size);
                            break;
                        case 2:
                            v = Vector3(cos(a) * size, 1, sin(a) * size);
                            break;
                        case 3:
                            v = Vector3(cos(a) * size, -1, sin(a) * size);
                            break;
                        case 4:
                            v = Vector3(cos(a) * size, sin(a) * size, 1);
                            break;
                        case 5:
                            v = Vector3(cos(a) * size, sin(a) * size, -1);
                            break;
                    }
                    v.normalize();
                    pAgent->initialize(v, genome.c_str(), true);
                    world.addAgentToWorld(pAgent);
                    pAgent->mStatus = eInanimate;
                    pAgent->mEnergy = pAgent->getSpawnEnergy();
                }
            }
        }
    }
    pthread_mutex_unlock( &mutex1 );
}


void Main :: updateControlLabel(std::string parameterId, const char *pFormat, ...)
{
    Label * pLabel = (Label*)_formMain->getControl((parameterId + "Label").c_str());
    if (! pLabel)
        pLabel = (Label*)_formAdvanced->getControl((parameterId + "Label").c_str());
    
    if (pLabel)
    {
        char buffer[1024];
        va_list argptr;
        va_start(argptr, pFormat);
        vsprintf(buffer, pFormat, argptr);
        va_end(argptr);
        
        pLabel->setText(buffer);
    }
}


void Main::updateControlLabels()
{
    
    updateControlLabel("speed", "%d", Parameters::speed);
    updateControlLabel("mutation", "%d%%", Parameters::mutationPercent);
    updateControlLabel("cellSize", "%d", (int) Parameters::cellSize);
    updateControlLabel("cycleEnergyCost", "-%.1f", Parameters::cycleEnergyCost);
    updateControlLabel("photoSynthesizeEnergyGain", "+%.1f", Parameters::photoSynthesizeEnergyGain);
    updateControlLabel("moveEnergyCost", "-%.1f", Parameters::moveEnergyCost);
    updateControlLabel("moveAndEatEnergyCost", "-%.1f", Parameters::moveAndEatEnergyCost);
    updateControlLabel("baseSpawnEnergy", "%d", (int) Parameters::baseSpawnEnergy);
    updateControlLabel("extraSpawnEnergyPerSegment", "%d", (int) Parameters::extraSpawnEnergyPerSegment);
    updateControlLabel("lookDistance", "%d", Parameters::lookDistance);
}

void Main::setControlValues()
{
    _speedSlider->setValue(Parameters::speed);
    _mutationSlider->setValue(Parameters::mutationPercent);
    _cellSizeSlider->setValue(Parameters::cellSize);
//    _cycleEnergyCostSlider->setValue(Parameters::cycleEnergyCost);
    _photoSynthesizeEnergyGainSlider->setValue(Parameters::photoSynthesizeEnergyGain);
    _moveEnergyCostSlider->setValue(Parameters::moveEnergyCost);
    _moveAndEatEnergyCostSlider->setValue(Parameters::moveAndEatEnergyCost);
    _baseSpawnEnergySlider->setValue(Parameters::baseSpawnEnergy);
    _extraSpawnEnergyPerSegmentSlider->setValue(Parameters::extraSpawnEnergyPerSegment);
    _lookDistanceSlider->setValue(Parameters::lookDistance);
    _allowSelfOverlap->setChecked(Parameters::allowSelfOverlap);
}


void Main::finalize()
{
    threadAlive = false;
    void *status;
    pthread_join (mThread, &status);
    
    SAFE_RELEASE(_scene);
    SAFE_RELEASE(_formAdvanced);
    SAFE_RELEASE(_formMain);
}

void Main::update(float elapsedTime)
{
    // Rotate model
    _formMain->update(elapsedTime);
    if (_showAdvanced->isChecked())
        _formAdvanced->update(elapsedTime);
    
    _formHelp->update(elapsedTime);
}

std::map<string, int> gMapSpeciesToCount;
vector<pair<string,int> > gTopSpecies;

bool compareTopSpeciesFunc(const pair<string,int> &p1, const pair<string,int> &p2);
bool compareTopSpeciesFunc(const pair<string,int> &p1, const pair<string,int> &p2)
{
    return p1.second > p2.second;
}


float gTurnsPerSecond = 0;

void Main::render(float elapsedTime)
{
    // Clear the color and depth buffers
    clear(CLEAR_COLOR_DEPTH, Vector4::zero(), 1.0f, 0);
    
    for (int i = 0; i < 255; i++)
        if (mSegmentBatch[i])
            mSegmentBatch[i]->start();
    
    float renderSize = std::min(getWidth(), getHeight()) * .9;
    float offsetX = (getWidth() - renderSize) / 2;
    float offsetY = (getHeight() - renderSize) / 2;

    mRenderSphereSize = renderSize * mViewScale * .995;
    mSphereOffsetX = (getWidth() - mRenderSphereSize) / 2;
    mSphereOffsetY = (getHeight() - mRenderSphereSize) / 2;

    _arcball.setBounds(mRenderSphereSize, mRenderSphereSize);
    
    mSegmentBatch[0]->draw(Rectangle(mSphereOffsetX, mSphereOffsetY, mRenderSphereSize, mRenderSphereSize), Rectangle(0,0,1024,1024));
    
    bool sampleCritters = false;
    
    static float elapsedTimeSinceTally = 1000;
    static int startSampleTurns = 0;
    
    elapsedTimeSinceTally += elapsedTime;
    
    if (elapsedTimeSinceTally > 1000)
    {
        int numTurnsSinceTally = numTurns - startSampleTurns;
        startSampleTurns = numTurns;
        
        gTurnsPerSecond = ((float) numTurnsSinceTally) / (elapsedTimeSinceTally/1000);
        
        sampleCritters = true;
        gMapSpeciesToCount.clear();
        gTopSpecies.clear();
        
        elapsedTimeSinceTally = 0;
    }
    
    for (int i = 0; i <= world.mMaxLiveAgentIndex; i++)
    {
        Agent & agent = world.mAgents[i];
        if (agent.mStatus != eNonExistent)
        {
            if ((agent.mStatus == eAlive) && (agent.mSleep != -1))
            {
                int count = gMapSpeciesToCount[agent.mGenome];
                ++count;
                gMapSpeciesToCount[agent.mGenome] = count;
            }
            
            for (int j = agent.mNumSegments-1; j >= 0; j--)
            {
                SphereEntity *pEntity = &agent.mSegments[j];
                Vector3 pt = pEntity->mLocation;
                mViewRotateMatrix.transformPoint(&pt);
                Matrix viewScaleMatrix;
                viewScaleMatrix.scale(mViewScale);
                viewScaleMatrix.transformPoint(&pt);

                if (pt.z < 0)
                    continue; // cheap backface clipping
                
                // we use an orthogonal projection, but with some fakery to make it look more 3D
                float cellSize = Parameters::getMoveDistance() * pt.z * 450;
                float x = ((pt.x) * (pt.z/5 + 1)) * .98;
                float y = ((pt.y) * (pt.z/5 + 1)) * .98;
                Rectangle dst = Rectangle(offsetX + renderSize * (x + 1) / 2, offsetY + renderSize * (y + 1) / 2,
                                          cellSize, cellSize);
                
                SpriteBatch * pBatch = mSegmentBatch[pEntity->mType];
                
                if (pBatch == mSegmentBatch[20])
                    printf("what?");
                
                Rectangle src = Rectangle(0,0,
                                          pBatch->getSampler()->getTexture()->getWidth(), pBatch->getSampler()->getTexture()->getHeight());
                
                float alpha = 1;
                if (agent.mStatus == eAlive)
                    alpha = (float)agent.mEnergy/(float)agent.getSpawnEnergy();
                pBatch->draw(dst, src, Vector4(1,1,1, alpha));
                
                // if we're zoomed in, show an indicator around the active cell
                if ((mViewScale > 2) &&
                    (j == ((agent.mActiveSegment + agent.mNumSegments - 1) % agent.mNumSegments)) &&
                    (agent.mNumSegments > 1))
                {
                    pBatch = mSegmentBatch[1];
                    src = Rectangle(0,0,
                                    pBatch->getSampler()->getTexture()->getWidth(), pBatch->getSampler()->getTexture()->getHeight());
                    
                    dst.inflate(cellSize / 4, cellSize / 4);
                    pBatch->draw(dst, src, Vector4(1,1,1, 1));
                }
                
            }
        }
    }
    
    
    if (sampleCritters)
    {
        for (std::map<string, int>::iterator i = gMapSpeciesToCount.begin(); i != gMapSpeciesToCount.end(); i++)
        {
            gTopSpecies.push_back(*i);//pair<string,int>((*i).first, (*i).second);
        }
        sort(gTopSpecies.begin(), gTopSpecies.end(), compareTopSpeciesFunc);
    }
    
    _font->start();
    
    for (int i = 0; i < gTopSpecies.size(); i++)
    {
        if (i == 33)
            break;
        char buf[200];
        float x = getWidth()-50;
        float y = 20 + 20 * i;
        
        sprintf(buf, "%d", gTopSpecies[i].second);
        _font->drawText(buf, x, y, Vector4(1,1,1,1));
        
        x -= 40;
        const char *pGenome = gTopSpecies[i].first.c_str();
        int len = strlen(pGenome);
        for (int j = (len - 1); j >= 0; j--)
        {
            char ch = pGenome[j];
            SpriteBatch * pBatch = mSegmentBatch[ch];
            Rectangle src(0,0,
                          pBatch->getSampler()->getTexture()->getWidth(), pBatch->getSampler()->getTexture()->getHeight());
            Rectangle dst(x, y, 16, 16);
            pBatch->draw(dst, src);
            x -= 18;
        }
    }
    
    totalElapsedTime += elapsedTime;
    
    if (gTurnsPerSecond > 0)
    {
        char buf[200];
        sprintf(buf, "Turns/sec: %d", (int) gTurnsPerSecond);
        _font->drawText(buf,  getWidth()-200, getHeight() - 25, Vector4(1,1,1,1));
    }
    
    for (int i = 0; i < 255; i++)
        if (mSegmentBatch[i])
            mSegmentBatch[i]->finish();
    _font->finish();
    
    // Draw the UI.
    _formMain->draw();
    if (_showAdvanced->isChecked())
        _formAdvanced->draw();
    
    _formHelp->draw();

}

bool Main::drawScene(Node* node)
{
    // If the node visited contains a model, draw it
    Model* model = node->getModel();
    if (model)
    {
        model->draw();
    }
    return true;
}

void Main::keyEvent(Keyboard::KeyEvent evt, int key)
{
    if (evt == Keyboard::KEY_PRESS)
    {
        switch (key)
        {
            case Keyboard::KEY_ESCAPE:
                exit();
                break;
        }
    }
}

void Main::touchEvent(Touch::TouchEvent evt, int x, int y, unsigned int contactIndex)
{
    static bool down = false;
    static int lastX, lastY;

    static Matrix rotateMatrix;
    static Matrix initialViewMatrix;
    
    Vector2 sphereDisplayPoint(x - mSphereOffsetX, mRenderSphereSize - (y - mSphereOffsetY));
    switch (evt)
    {
        case Touch::TOUCH_PRESS:
            _arcball.click(sphereDisplayPoint);
            rotateMatrix.identity();
            initialViewMatrix = mViewRotateMatrix;
            break;
            
        case Touch::TOUCH_RELEASE:
            down = false;
            break;
            
        case Touch::TOUCH_MOVE: {
            gameplay::Quaternion q;
            _arcball.drag(sphereDisplayPoint, &q);            
            Matrix::createRotation(q, &rotateMatrix);
            rotateMatrix.multiply(initialViewMatrix);
            mViewRotateMatrix = rotateMatrix;
            break; }
    };
}

void Main::gesturePinchEvent(int x, int y, float scale)
{
#if 0
    mViewScale *= scale;
#else
    mViewScale += scale;
#endif
    
    if (mViewScale > 5)
        mViewScale = 5;
    if (mViewScale < .5)
        mViewScale = .5;
}
