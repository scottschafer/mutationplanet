#ifndef TEMPLATEGAME_H_
#define TEMPLATEGAME_H_

#include "gameplay.h"
#include "arcball.h"

using namespace gameplay;

/**
 * Main game class.
 */
class Main: public Game, Control::Listener

{
public:

    /**
     * Constructor.
     */
    Main();

    /**
     * @see Game::keyEvent
     */
	void keyEvent(Keyboard::KeyEvent evt, int key);
	
    /**
     * @see Game::touchEvent
     */
    void touchEvent(Touch::TouchEvent evt, int x, int y, unsigned int contactIndex);
    void gesturePinchEvent(int x, int y, float scale);

protected:

    /**
     * @see Game::initialize
     */
    void initialize();
    void createUI();
    
    /**
     * @see Game::finalize
     */
    void finalize();

    /**
     * @see Game::update
     */
    void update(float elapsedTime);

    /**
     * @see Game::render
     */
    void render(float elapsedTime);

    /**
     * @see Control::controlEvent
     */
    void controlEvent(Control* control, EventType evt);

    void updateControlLabels();
    void setControlValues();
    void createControlHeader(Form *form, std::string text);
    void createSpacer(Form *form, float height);
    Slider * createSliderControl(Form *form, std::string id, std::string label, float minValue, float maxValue, float step = 0);
    CheckBox * createCheckboxControl(Form *form, std::string label);
    Button * createButton(Form *form, std::string label);

    void updateControlLabel(std::string parameterId, const char *pFormat, ...);

    void setBarriers(int type, bool bOn);
    
    void reset();

private:
    static void * threadFunction(void*);

private:
    ArcBall _arcball;
    pthread_t mThread;
    
    float mRenderSphereSize;
    float mSphereOffsetX;
    float mSphereOffsetY;
    
    float  mViewScale;
    Matrix mViewRotateMatrix;

    /**
     * Draws the scene each frame.
     */
    bool drawScene(Node* node);

    Scene* _scene;
    Form* _formMain;
    Form* _formAdvanced;
    Form* _formHelp;
    Font* _font;

    Button * _resetButton;
    Button * _helpButton;
    
    CheckBox * _barriers1;
    CheckBox * _barriers2;
    CheckBox * _barriers3;

    CheckBox * _showAdvanced;
    Slider* _speedSlider;
    Slider* _mutationSlider;
    Slider* _cellSizeSlider;

    Slider* _cycleEnergyCostSlider;
    Slider* _photoSynthesizeEnergyGainSlider;
    Slider* _moveEnergyCostSlider;
    Slider* _moveAndEatEnergyCostSlider;

    Slider* _baseSpawnEnergySlider;
    Slider* _extraSpawnEnergyPerSegmentSlider;

    Slider* _lookDistanceSlider;
    CheckBox * _allowSelfOverlap;

    SpriteBatch * mSegmentBatch[255];
};

#endif
