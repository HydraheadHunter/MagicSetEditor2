//+----------------------------------------------------------------------------+
//| Description:  Magic Set Editor - Program to make Magic (tm) cards          |
//| Copyright:    (C) Twan van Laarhoven and the other MSE developers          |
//| License:      GNU General Public License 2 or later (see file COPYING)     |
//+----------------------------------------------------------------------------+

#pragma once

// ----------------------------------------------------------------------------- : Includes

#include <util/prec.hpp>
#include <util/rotation.hpp>
#include <util/real_point.hpp>
#include <data/draw_what.hpp>
#include <data/field.hpp>

class Set;
class Package;
class DataViewer;
class Action;
DECLARE_POINTER_TYPE(Style);
DECLARE_POINTER_TYPE(Value);

// ----------------------------------------------------------------------------- : ValueViewer

/// The virtual viewer control for a single field on a card (or in the set data)
/** A viewer can only display a value, not edit it, ValueEditor is used for that */
class ValueViewer : public StyleListener {
public:
  /// Construct a ValueViewer, set the value at a later time
  ValueViewer(DataViewer& parent, const StyleP& style);
  virtual ~ValueViewer() {}
  
  /// Change the associated value
  void setValue(const ValueP&);
  /// Return the associated field
  inline const FieldP& getField() const { return styleP->fieldP; }
  /// Return the associated style
  inline const StyleP& getStyle() const { return styleP; }
  /// Return the associated value
  inline const ValueP& getValue() const { return valueP; }
  
  /// Prepare before drawing.
  /** Should return true if a content property has changed
   *  Scripts are re-updated after preparing if they depend on content properties. */
  virtual bool prepare(RotatedDC& dc) { return false; };
  /// Draw this value
  virtual void draw(RotatedDC& dc) = 0;
  
  /// Does this field contian the given point?
  virtual bool containsPoint(const RealPoint& p) const;
  /// Get a bounding rectangle for this field (including any border it may have)
  virtual RealRect boundingBoxBorder() const;
  /// Is this field visible?
  bool isVisible() const;
  
  /// Rotation to use for drawing this field
  virtual Rotation getRotation() const;
  /// Stretch factor
  virtual double getStretch() const { return 1.0; }
  
  /// Called when the associated value is changed
  /** Both when we are associated with another value,
   *  and by default when the value itself changes (called from onAction)
   */
  virtual void onValueChange() {}
  /// Called when a (scripted) property of the associated style has changed
  /** Default: redraws the viewer if needed */
  void onStyleChange(int changes) override;
  /// Called when an action is performed on the associated value
  virtual void onAction(const Action&, bool undone) { onValueChange(); }
  
  /// Convert this viewer to an editor, if possible
  virtual ValueEditor* getEditor() { return nullptr; }

public:
  DataViewer& parent; ///< Our parent object
  RealRect bounding_box; ///< The bounding box of this viewer. Corresponds to styleP->getExternalRect(), except for native look editor
protected:
  ValueP valueP; ///< The value we are currently viewing
  
  /// Set the pen for drawing the border, returns true if a border needs to be drawn
  bool setFieldBorderPen(RotatedDC& dc);
  /// Draws a border around the field
  void drawFieldBorder(RotatedDC& dc);
  
  /// Redraw this viewer
  void redraw();
  
  /// Load the AlphaMask for this field, scaled but not rotated
  const AlphaMask& getMask(int w = 0, int h = 0) const;
  const AlphaMask& getMask(const Rotation& rot) const;

public:
  // Context to use for script functions
  Context& getContext() const;

  /// Should this viewer render using a platform native look?
  bool nativeLook() const;
  /// What elements to draw
  DrawWhat drawWhat() const;
  /// Is this the currently selected viewer?
  /** Usually only the editor allows selection of viewers */
  bool isCurrent() const;
  
  /// The package containing style stuff like images
  Package& getStylePackage() const;
  /// The local package for loading/saving files
  Package& getLocalPackage() const;
};

// ----------------------------------------------------------------------------- : Utility

#define DECLARE_VALUE_VIEWER(Type) \
  protected: \
    inline       Type##Style& style()  const { return static_cast<      Type##Style&>(*ValueViewer::styleP); } \
    inline const Type##Value& value()  const { return static_cast<const Type##Value&>(*ValueViewer::valueP); } \
    inline const Type##Field& field()  const { return style().field(); } \
    inline       Type##StyleP styleP() const { return static_pointer_cast<Type##Style>(ValueViewer::styleP); } \
    inline       Type##ValueP valueP() const { return static_pointer_cast<Type##Value>(ValueViewer::valueP); } \
    inline       Type##FieldP fieldP() const { return static_pointer_cast<Type##Field>(style().fieldP); } \
  public: \
    Type##ValueViewer(DataViewer& parent, const Type ## StyleP& style)

#define IMPLEMENT_VALUE_VIEWER(Type) \
  ValueViewerP Type##Style::makeViewer(DataViewer& parent) { \
    return ValueViewerP(new Type##ValueViewer(parent, static_pointer_cast<Type##Style>(intrusive_from_this()))); \
  }
