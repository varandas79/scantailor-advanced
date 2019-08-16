// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef ZOOM_HANDLER_H_
#define ZOOM_HANDLER_H_

#include <QCoreApplication>
#include <QPoint>
#include <boost/function.hpp>
#include "InteractionHandler.h"
#include "InteractionState.h"

class ImageViewBase;

class ZoomHandler : public InteractionHandler {
  Q_DECLARE_TR_FUNCTIONS(ZoomHandler)
 public:
  enum Focus { CENTER, CURSOR };

  explicit ZoomHandler(ImageViewBase& image_view);

  ZoomHandler(ImageViewBase& image_view,
              const boost::function<bool(const InteractionState&)>& explicit_interaction_permitter);

  Focus focus() const { return m_focus; }

  void setFocus(Focus focus) { m_focus = focus; }

 protected:
  void onWheelEvent(QWheelEvent* event, InteractionState& interaction) override;

  void onKeyPressEvent(QKeyEvent* event, InteractionState& interaction) override;

 private:
  ImageViewBase& m_imageView;
  boost::function<bool(const InteractionState&)> m_interactionPermitter;
  InteractionState::Captor m_interaction;
  Focus m_focus;
};


#endif  // ifndef ZOOM_HANDLER_H_
