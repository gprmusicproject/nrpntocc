#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

//==============================================================================
// GPR music project brand tokens ("Studio notturno", same as gprmusic.com):
// dark surfaces, one amber accent, no glows or shadows.
namespace brand
{
    const juce::Colour bg         { 0xff0D0E11 };
    const juce::Colour surface    { 0xff121418 };
    const juce::Colour card       { 0xff16181D };
    const juce::Colour raised     { 0xff1A1C22 };
    const juce::Colour line       { 0xff262930 };
    const juce::Colour lineStrong { 0xff3A3F48 };
    const juce::Colour text       { 0xffEEE9E0 };
    const juce::Colour textMuted  { 0xffB9B4AB };
    const juce::Colour textFaint  { 0xff8C887F };
    const juce::Colour accent     { 0xffF2A33A };

    inline juce::Font monoFont (float height)
    {
        return juce::Font (juce::FontOptions{}.withName (juce::Font::getDefaultMonospacedFontName())
                                              .withHeight (height));
    }
}

//==============================================================================
// Value box of the sliders, drawn like the combo boxes: card fill, radius 8,
// outline that turns amber while hovered or being edited.
class BrandTextBox : public juce::Label
{
public:
    void paint (juce::Graphics& g) override
    {
        const auto r = getLocalBounds().toFloat().reduced (0.5f);
        g.setColour (brand::card);
        g.fillRoundedRectangle (r, 8.0f);
        g.setColour (isMouseOver (true) || isBeingEdited() ? brand::accent : brand::lineStrong);
        g.drawRoundedRectangle (r, 8.0f, 1.0f);

        juce::Label::paint (g);
    }

    void mouseEnter (const juce::MouseEvent& e) override { repaint(); juce::Label::mouseEnter (e); }
    void mouseExit  (const juce::MouseEvent& e) override { repaint(); juce::Label::mouseExit (e); }
};

//==============================================================================
class BrandLookAndFeel : public juce::LookAndFeel_V4
{
public:
    BrandLookAndFeel()
        : juce::LookAndFeel_V4 ({ brand::bg,          // windowBackground
                                  brand::card,        // widgetBackground
                                  brand::card,        // menuBackground
                                  brand::lineStrong,  // outline
                                  brand::text,        // defaultText
                                  brand::accent,      // defaultFill
                                  brand::bg,          // highlightedText
                                  brand::accent,      // highlightedFill
                                  brand::text })      // menuText
    {
        using namespace juce;

        setColour (ResizableWindow::backgroundColourId,   brand::bg);
        setColour (Label::textColourId,                   brand::text);

        setColour (ComboBox::backgroundColourId,          brand::card);
        setColour (ComboBox::outlineColourId,             brand::lineStrong);
        setColour (ComboBox::arrowColourId,               brand::textMuted);
        setColour (ComboBox::textColourId,                brand::text);

        setColour (PopupMenu::backgroundColourId,         brand::card);
        setColour (PopupMenu::textColourId,               brand::text);
        setColour (PopupMenu::highlightedBackgroundColourId, brand::accent);
        setColour (PopupMenu::highlightedTextColourId,    brand::bg);

        setColour (TextButton::buttonColourId,            brand::card);
        setColour (TextButton::buttonOnColourId,          brand::accent);
        setColour (TextButton::textColourOffId,           brand::text);
        setColour (TextButton::textColourOnId,            brand::bg);

        setColour (ToggleButton::textColourId,            brand::text);
        setColour (ToggleButton::tickColourId,            brand::accent);
        setColour (ToggleButton::tickDisabledColourId,    brand::textFaint);

        setColour (Slider::textBoxBackgroundColourId,     brand::card);
        setColour (Slider::textBoxOutlineColourId,        brand::lineStrong);
        setColour (Slider::textBoxTextColourId,           brand::text);
        setColour (Slider::textBoxHighlightColourId,      brand::accent.withAlpha (0.35f));

        setColour (TextEditor::backgroundColourId,        brand::card);
        setColour (TextEditor::textColourId,              brand::text);
        setColour (TextEditor::outlineColourId,           brand::lineStrong);
        setColour (TextEditor::focusedOutlineColourId,    brand::accent);
        setColour (TextEditor::highlightColourId,         brand::accent.withAlpha (0.35f));
        setColour (CaretComponent::caretColourId,         brand::accent);

        setColour (HyperlinkButton::textColourId,         brand::textFaint);
    }

    // Combo boxes: card fill, soft corners (radius 8), amber outline on hover.
    void drawComboBox (juce::Graphics& g, int width, int height, bool,
                       int, int, int, int, juce::ComboBox& box) override
    {
        const auto r = juce::Rectangle<float> (0.0f, 0.0f, (float) width, (float) height).reduced (0.5f);

        g.setColour (box.findColour (juce::ComboBox::backgroundColourId));
        g.fillRoundedRectangle (r, 8.0f);

        g.setColour (box.isMouseOver (true) ? brand::accent : box.findColour (juce::ComboBox::outlineColourId));
        g.drawRoundedRectangle (r, 8.0f, 1.0f);

        // Chevron on the right.
        const auto a = juce::Rectangle<float> ((float) width - 26.0f, 0.0f, 16.0f, (float) height)
                           .withSizeKeepingCentre (10.0f, 6.0f);
        juce::Path chevron;
        chevron.startNewSubPath (a.getX(), a.getY());
        chevron.lineTo (a.getCentreX(), a.getBottom());
        chevron.lineTo (a.getRight(), a.getY());
        g.setColour (box.findColour (juce::ComboBox::arrowColourId));
        g.strokePath (chevron, juce::PathStrokeType (1.6f, juce::PathStrokeType::curved,
                                                     juce::PathStrokeType::rounded));
    }

    void positionComboBoxText (juce::ComboBox& box, juce::Label& label) override
    {
        label.setBounds (10, 1, box.getWidth() - 36, box.getHeight() - 2);
        label.setFont (getComboBoxFont (box));
    }

    // Inc/dec sliders: a gap between the value box and the - / + buttons.
    juce::Slider::SliderLayout getSliderLayout (juce::Slider& slider) override
    {
        auto layout = juce::LookAndFeel_V4::getSliderLayout (slider);
        if (slider.getSliderStyle() == juce::Slider::IncDecButtons)
            layout.sliderBounds.removeFromLeft (8);
        return layout;
    }

    juce::Label* createSliderTextBox (juce::Slider& slider) override
    {
        auto* l = new BrandTextBox();
        l->setJustificationType (juce::Justification::centred);
        l->setKeyboardType (juce::TextInputTarget::decimalKeyboard);
        l->setColour (juce::Label::textColourId,           slider.findColour (juce::Slider::textBoxTextColourId));
        l->setColour (juce::Label::backgroundColourId,     juce::Colours::transparentBlack);
        l->setColour (juce::Label::outlineColourId,        juce::Colours::transparentBlack);
        l->setColour (juce::TextEditor::textColourId,      brand::text);
        l->setColour (juce::TextEditor::backgroundColourId, juce::Colours::transparentBlack);
        l->setColour (juce::TextEditor::outlineColourId,   juce::Colours::transparentBlack);
        l->setColour (juce::TextEditor::focusedOutlineColourId, juce::Colours::transparentBlack);
        l->setColour (juce::TextEditor::highlightColourId, brand::accent.withAlpha (0.35f));
        return l;
    }

    // Buttons: pill shape, outline that turns amber on hover (site button style).
    void drawButtonBackground (juce::Graphics& g, juce::Button& button, const juce::Colour&,
                               bool isHighlighted, bool isDown) override
    {
        const auto r = button.getLocalBounds().toFloat().reduced (0.5f);
        const float radius = r.getHeight() * 0.5f;

        g.setColour (isDown ? brand::raised : brand::card);
        g.fillRoundedRectangle (r, radius);

        g.setColour (isHighlighted || isDown ? brand::accent : brand::lineStrong);
        g.drawRoundedRectangle (r, radius, 1.0f);
    }

    // Check boxes: amber filled box with a dark tick when on, outline when off.
    void drawTickBox (juce::Graphics& g, juce::Component& c, float x, float y, float w, float h,
                      bool ticked, bool isEnabled, bool isHighlighted, bool) override
    {
        const auto box = juce::Rectangle<float> (x, y, w, h).reduced (1.5f);

        if (ticked)
        {
            g.setColour (isEnabled ? brand::accent : brand::textFaint);
            g.fillRoundedRectangle (box, 4.0f);

            juce::Path tick;
            tick.startNewSubPath (box.getX() + box.getWidth() * 0.24f, box.getY() + box.getHeight() * 0.52f);
            tick.lineTo (box.getX() + box.getWidth() * 0.43f, box.getY() + box.getHeight() * 0.72f);
            tick.lineTo (box.getX() + box.getWidth() * 0.78f, box.getY() + box.getHeight() * 0.30f);
            g.setColour (brand::bg);
            g.strokePath (tick, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved,
                                                      juce::PathStrokeType::rounded));
        }
        else
        {
            g.setColour (isHighlighted ? brand::accent : c.findColour (juce::ToggleButton::tickDisabledColourId));
            g.drawRoundedRectangle (box, 4.0f, 1.0f);
        }
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BrandLookAndFeel)
};
