//
//  ScalableSlider.h
//  MutationPlanet
//
//  Created by Scott Schafer on 4/16/13.
//
//

#ifndef __MutationPlanet__ScalableSlider__
#define __MutationPlanet__ScalableSlider__

//#include "Base.h"
#include "Theme.h"
#include "Properties.h"
#include "Button.h"
#include "Touch.h"
#include "Slider.h"

namespace gameplay
{
    class ScalableSlider : public Slider
    {
    public:
        static ScalableSlider* create(const char* id, Theme::Style* style, float scaling);
        
    protected:
        ScalableSlider(float scaling);
        void drawImages(SpriteBatch* spriteBatch, const Rectangle& clip);

    private:
        float mScaling;
    };
};

#endif /* defined(__MutationPlanet__ScalableSlider__) */
