//+----------------------------------------------------------------------------+
//| Description:  Magic Set Editor - Program to make Magic (tm) cards          |
//| Copyright:    (C) Twan van Laarhoven and the other MSE developers          |
//| License:      GNU General Public License 2 or later (see file COPYING)     |
//+----------------------------------------------------------------------------+

// ----------------------------------------------------------------------------- : Includes

#include <util/prec.hpp>
#include <gui/control/image_card_list.hpp>
#include <gui/thumbnail_thread.hpp>
#include <data/field/image.hpp>
#include <data/game.hpp>
#include <data/card.hpp>
#include <gfx/gfx.hpp>
#include <wx/imaglist.h>
#include <gui/util.hpp>

// ----------------------------------------------------------------------------- : ImageCardList

ImageCardList::ImageCardList(Window* parent, int id, long additional_style)
  : CardListBase(parent, id, additional_style)
{}

ImageCardList::~ImageCardList() {
  thumbnail_thread.abort(this);
}
void ImageCardList::onRebuild() {
  image_field = findImageField();
}
void ImageCardList::onBeforeChangeSet() {
  CardListBase::onBeforeChangeSet();
  // remove all but the first two (sort asc/desc) images from image list
  wxImageList* il = GetImageList(wxIMAGE_LIST_SMALL);
  while (il && il->GetImageCount() > 2) {
    il->Remove(2);
  }
  thumbnail_thread.abort(this);
  thumbnails.clear();
}

ImageFieldP ImageCardList::findImageField() {
  FOR_EACH(f, set->game->card_fields) {
    ImageFieldP imgf = dynamic_pointer_cast<ImageField>(f);
    if (imgf) return imgf;
  }
  return ImageFieldP();
}

/// A request for a thumbnail of a card image
class CardThumbnailRequest : public ThumbnailRequest {
public:
  CardThumbnailRequest(ImageCardList* parent, const LocalFileName& filename)
    : ThumbnailRequest(
      parent,
      _("card") + parent->set->absoluteFilename() + _("-") + filename.toStringForKey(),
      wxDateTime::Now())  // TODO: Find mofication time of card image
    , filename(filename)
  {}
  Image generate() override {
    try {
      ImageCardList* parent = (ImageCardList*)owner;
      Image image;
      if (image_load_file(image, *parent->set->openIn(filename))) {
        // two step anti aliased resampling
        image.Rescale(36, 28); // step 1: no anti aliassing
        return resample(image, 18, 14); // step 2: with anti aliassing
      } else {
        return Image();
      }
    } catch (...) {
      return Image();
    }
  }
  void store(const Image& img) override {
    // add finished bitmap to the imagelist
    ImageCardList* parent = (ImageCardList*)owner;
    if (img.Ok()) {
      wxImageList* il = parent->GetImageList(wxIMAGE_LIST_SMALL);
      int id = il->Add(wxBitmap(img));
      parent->thumbnails.insert(make_pair(filename.toStringForKey(), id));
      parent->Refresh(false);
    }
  }

  bool threadSafe() const override {return true;}
private:
  LocalFileName filename;
};

int ImageCardList::OnGetItemImage(long pos) const {
  if (image_field) {
    // Image = thumbnail of first image field of card
    ImageValue& val = static_cast<ImageValue&>(*getCard(pos)->data[image_field]);
    if (val.filename.empty()) return -1; // no image
    // is there already a thumbnail?
    map<String,int>::const_iterator it = thumbnails.find(val.filename.toStringForKey());
    if (it != thumbnails.end()) {
      return it->second;
    } else {
      // request a thumbnail
      thumbnail_thread.request(make_intrusive<CardThumbnailRequest>(const_cast<ImageCardList*>(this), val.filename));
    }
  }
  return -1;
}

void ImageCardList::onIdle(wxIdleEvent&) {
  thumbnail_thread.done(this);
}


BEGIN_EVENT_TABLE(ImageCardList, CardListBase)
  EVT_IDLE     (ImageCardList::onIdle)
END_EVENT_TABLE  ()

// ----------------------------------------------------------------------------- : FilteredImageCardList

FilteredImageCardList::FilteredImageCardList(Window* parent, int id, long additional_style)
  : ImageCardList(parent, id, additional_style)
{}

void FilteredImageCardList::setFilter(const CardListFilterP& filter) {
  this->filter = filter;
  rebuild();
}

void FilteredImageCardList::onChangeSet() {
  // clear filter before changing set, the filter might not make sense for a different set
  filter = CardListFilterP();
  CardListBase::onChangeSet();
}

void FilteredImageCardList::getItems(vector<VoidP>& out) const {
  if (filter) {
    filter->getItems(set->cards,out);
  } else {
    ImageCardList::getItems(out);
  }
}
