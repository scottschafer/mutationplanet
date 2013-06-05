//
//  ScalableSlider.cpp
//  MutationPlanet
//
//  Created by Scott Schafer on 4/16/13.
//
//

#include "ScalableSlider.h"

using namespace gameplay;

ScalableSlider* ScalableSlider::create(const char* id, Theme::Style* style, float scaling)
{
    GP_ASSERT(style);
    
    ScalableSlider* slider = new ScalableSlider(scaling);
    if (id)
        slider->_id = id;
    slider->setStyle(style);
    
    return slider;
}

ScalableSlider::ScalableSlider(float scaling)
{
    mScaling = scaling;
}

void ScalableSlider::drawImages(SpriteBatch* spriteBatch, const Rectangle& clip)
{
    GP_ASSERT(spriteBatch);
    GP_ASSERT(_minImage);
    GP_ASSERT(_maxImage);
    GP_ASSERT(_markerImage);
    GP_ASSERT(_trackImage);
    
    // TODO: Vertical slider.
    
    // The slider is drawn in the center of the control (perpendicular to orientation).
    // The track is stretched according to orientation.
    // Caps and marker are not stretched.
    const Rectangle& minCapRegion = _minImage->getRegion();
    const Rectangle& maxCapRegion = _maxImage->getRegion();
    const Rectangle& markerRegion = _markerImage->getRegion();
    const Rectangle& trackRegion = _trackImage->getRegion();
    
    const Theme::UVs& minCap = _minImage->getUVs();
    const Theme::UVs& maxCap = _maxImage->getUVs();
    const Theme::UVs& marker = _markerImage->getUVs();
    const Theme::UVs& track = _trackImage->getUVs();
    
    Vector4 minCapColor = _minImage->getColor();
    Vector4 maxCapColor = _maxImage->getColor();
    Vector4 markerColor = _markerImage->getColor();
    Vector4 trackColor = _trackImage->getColor();
    
    minCapColor.w *= _opacity;
    maxCapColor.w *= _opacity;
    markerColor.w *= _opacity;
    trackColor.w *= _opacity;
    
    // Draw order: track, caps, marker.
    float midY = _viewportBounds.y + (_viewportBounds.height) * 0.5f;
    Vector2 pos(_viewportBounds.x, midY - trackRegion.height * 0.5f);
    spriteBatch->draw(pos.x, pos.y, _viewportBounds.width, trackRegion.height, track.u1, track.v1, track.u2, track.v2, trackColor, _viewportClipBounds);
    
    pos.y = midY - minCapRegion.height * 0.5f;
    pos.x -= minCapRegion.width * 0.5f;
    spriteBatch->draw(pos.x, pos.y, minCapRegion.width, minCapRegion.height, minCap.u1, minCap.v1, minCap.u2, minCap.v2, minCapColor, _viewportClipBounds);
    
    pos.x = _viewportBounds.x + _viewportBounds.width - maxCapRegion.width * 0.5f;
    spriteBatch->draw(pos.x, pos.y, maxCapRegion.width, maxCapRegion.height, maxCap.u1, maxCap.v1, maxCap.u2, maxCap.v2, maxCapColor, _viewportClipBounds);
    
    // Percent across.
    float markerPosition = (_value - _min) / (_max - _min);
    markerPosition *= _viewportBounds.width - minCapRegion.width * 0.5f - maxCapRegion.width * 0.5f - markerRegion.width;
    pos.x = _viewportBounds.x + minCapRegion.width * 0.5f + markerPosition;
    pos.y = midY - markerRegion.height * mScaling / 2.0f;
    spriteBatch->draw(pos.x, pos.y, markerRegion.width * mScaling, markerRegion.height * mScaling, marker.u1, marker.v1, marker.u2, marker.v2, markerColor, _viewportClipBounds);
}
