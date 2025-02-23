/*
 * This file is part of OpenModelica.
 *
 * Copyright (c) 1998-CurrentYear, Open Source Modelica Consortium (OSMC),
 * c/o Linköpings universitet, Department of Computer and Information Science,
 * SE-58183 Linköping, Sweden.
 *
 * All rights reserved.
 *
 * THIS PROGRAM IS PROVIDED UNDER THE TERMS OF GPL VERSION 3 LICENSE OR
 * THIS OSMC PUBLIC LICENSE (OSMC-PL) VERSION 1.2.
 * ANY USE, REPRODUCTION OR DISTRIBUTION OF THIS PROGRAM CONSTITUTES RECIPIENT'S ACCEPTANCE
 * OF THE OSMC PUBLIC LICENSE OR THE GPL VERSION 3, ACCORDING TO RECIPIENTS CHOICE.
 *
 * The OpenModelica software and the Open Source Modelica
 * Consortium (OSMC) Public License (OSMC-PL) are obtained
 * from OSMC, either from the above address,
 * from the URLs: http://www.ida.liu.se/projects/OpenModelica or
 * http://www.openmodelica.org, and in the OpenModelica distribution.
 * GNU version 3 is obtained from: http://www.gnu.org/copyleft/gpl.html.
 *
 * This program is distributed WITHOUT ANY WARRANTY; without
 * even the implied warranty of  MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE, EXCEPT AS EXPRESSLY SET FORTH
 * IN THE BY RECIPIENT SELECTED SUBSIDIARY LICENSE CONDITIONS OF OSMC-PL.
 *
 * See the full OSMC Public License conditions for more details.
 *
 */
/*
 * @author Adeel Asghar <adeel.asghar@liu.se>
 */

#include <QOpenGLContext> // must be included before OSG headers

#include <osgGA/MultiTouchTrackballManipulator>
#include <osgViewer/ViewerEventHandlers>

#include <QPainter>
#include <QColorDialog>
#include <QInputDialog>
#include <QKeyEvent>
#include <cassert>
#include <QtMath>
#include <QApplication>

#include "ViewerWidget.h"
#include "Modeling/MessagesWidget.h"

/*!
 * \brief Viewer::setUpThreading
 */
void Viewer::setUpThreading()
{
  if (_threadingModel == SingleThreaded) {
    if (_threadsRunning) {
      stopThreading();
    }
  } else {
    if (!_threadsRunning) {
      startThreading();
    }
  }
}

/*!
 * \class ViewerWidget
 * \brief Viewer widget for OpenSceneGraph animations.
 */
/*!
 * \brief ViewerWidget::ViewerWidget
 * \param parent
 * \param flags
 */
ViewerWidget::ViewerWidget(QWidget* parent, Qt::WindowFlags flags)
  : GLWidget(parent, flags)
{
  // Set the number of samples used for multisampling
#if (QT_VERSION >= QT_VERSION_CHECK(5, 4, 0))
  QSurfaceFormat format;
  format.setSamples(4);
  setFormat(format);
#else
  QGLFormat format;
  format.setSamples(4);
  setFormat(format);
#endif
  mpGraphicsWindow = new osgViewer::GraphicsWindowEmbedded(x(), y(), width(), height());
  mpViewer = new Viewer;
  mpSceneView = new osgViewer::View();
  mpAnimationWidget = qobject_cast<AbstractAnimationWindow*>(parent);
  // add a scene to viewer
  mpViewer->addView(mpSceneView);
  // get the viewer widget
  osg::ref_ptr<osg::Camera> camera = mpSceneView->getCamera();
  camera->setGraphicsContext(mpGraphicsWindow);
  camera->setClearColor(osg::Vec4(0.95, 0.95, 0.95, 1.0));
  camera->setViewport(new osg::Viewport(0, 0, width(), height()));
  camera->setProjectionMatrixAsPerspective(30.0f, static_cast<double>(width()/2) / static_cast<double>(height()/2), 1.0f, 10000.0f);
  mpSceneView->addEventHandler(new osgViewer::StatsHandler());
  // reverse the mouse wheel zooming
  osgGA::MultiTouchTrackballManipulator *pMultiTouchTrackballManipulator = new osgGA::MultiTouchTrackballManipulator();
  pMultiTouchTrackballManipulator->setWheelZoomFactor(-pMultiTouchTrackballManipulator->getWheelZoomFactor());
  mpSceneView->setCameraManipulator(pMultiTouchTrackballManipulator);
  // the osg threading model
  mpViewer->setThreadingModel(osgViewer::CompositeViewer::SingleThreaded);
  // disable the default setting of viewer.done() by pressing Escape.
  mpViewer->setKeyEventSetsDone(0);
  mpViewer->realize();
  // This ensures that the widget will receive keyboard events. This focus
  // policy is not set by default. The default, Qt::NoFocus, will result in
  // keyboard events that are ignored.
  setFocusPolicy(Qt::StrongFocus);
  setMinimumSize(100, 100);
  // Ensures that the widget receives mouse move events even though no
  // mouse button has been pressed. We require this in order to let the
  // graphics window switch viewports properly.
  setMouseTracking(true);
}

/*!
 * \brief ViewerWidget::paintEvent
 * Reimplementation of the paintEvent.\n
 * \sa ViewerWidget::paintGL()
 */
void ViewerWidget::paintEvent(QPaintEvent* /* paintEvent */)
{
  makeCurrent();
  QPainter painter(this);
  painter.setRenderHint(QPainter::Antialiasing);
  paintGL();
  painter.end();
  doneCurrent();
}

/*!
 * \brief ViewerWidget::paintGL
 * Renders the animation frame.
 * \sa ViewerWidget::paintEvent()
 */
void ViewerWidget::paintGL()
{
  mpViewer->frame();
}

/*!
 * \brief ViewerWidget::resizeGL
 * Resizes the graphics window.
 * \param width
 * \param height
 */
void ViewerWidget::resizeGL(int width, int height)
{
  int pixelRatio = qCeil(qApp->devicePixelRatio());
  getEventQueue()->windowResize(x() * pixelRatio, y() * pixelRatio, width * pixelRatio, height * pixelRatio);
  mpGraphicsWindow->resized(x() * pixelRatio, y() * pixelRatio, width * pixelRatio, height * pixelRatio);
}

/*!
 * \brief ViewerWidget::keyPressEvent
 * Passes the QWidget::keyPressEvent() to Graphics Window.
 * \param event
 */
void ViewerWidget::keyPressEvent(QKeyEvent *event)
{
  QString keyString = event->text();
  const char* keyData = keyString.toLocal8Bit().data();
  getEventQueue()->keyPress(osgGA::GUIEventAdapter::KeySymbol(*keyData));
}

/*!
 * \brief ViewerWidget::keyReleaseEvent
 * Passes the QWidget::keyReleaseEvent() to Graphics Window.
 * \param event
 */
void ViewerWidget::keyReleaseEvent(QKeyEvent *event)
{
  QString keyString = event->text();
  const char* keyData = keyString.toLocal8Bit().data();
  getEventQueue()->keyRelease(osgGA::GUIEventAdapter::KeySymbol(*keyData));
}

/*!
 * \brief ViewerWidget::mouseMoveEvent
 * Passes the QWidget::mouseMoveEvent() to Graphics Window.
 * \param event
 */
void ViewerWidget::mouseMoveEvent(QMouseEvent *event)
{
  int pixelRatio = qCeil(qApp->devicePixelRatio());
  getEventQueue()->mouseMotion(static_cast<float>(event->x() * pixelRatio), static_cast<float>(event->y() * pixelRatio));
}

/*!
 * \brief ViewerWidget::mousePressEvent
 * Passes the QWidget::mousePressEvent() to Graphics Window.
 * \param event
 */
void ViewerWidget::mousePressEvent(QMouseEvent *event)
{
  // 1 = left mouse button
  // 2 = middle mouse button
  // 3 = right mouse button
  unsigned int button = 0;
  switch (event->button()) {
    case Qt::LeftButton:
      button = 1;
      break;
    case Qt::MiddleButton:
      button = 2;
      break;
    case Qt::RightButton:
      button = 3;
      if (event->modifiers() == Qt::ShiftModifier) {
        //qt counts pixels from upper left corner and osg from bottom left corner
        pickVisualizer(event->x(), this->height() - event->y());
        showVisualizerPickContextMenu(event->pos());
        return;
      }
      break;
    default:
      break;
  }
  int pixelRatio = qCeil(qApp->devicePixelRatio());
  getEventQueue()->mouseButtonPress(static_cast<float>(event->x() * pixelRatio), static_cast<float>(event->y() * pixelRatio), button);
}

/*!
 * \brief ViewerWidget::pickVisualizer
 * Picks the name of the selected visualizer in the osg view
 * \param x - mouse position pixel in x direction in osg system
 * \param y - mouse position pixel in y direction in osg system
 */
void ViewerWidget::pickVisualizer(int x, int y)
{
  //std::cout<<"pickVisualizer "<<x<<" and "<<y<<std::endl;
  osgUtil::LineSegmentIntersector::Intersections intersections;
  if (mpSceneView->computeIntersections(mpSceneView->getCamera(), osgUtil::Intersector::WINDOW, x, y, intersections)) {
    //take the first intersection with a facette only
    osgUtil::LineSegmentIntersector::Intersections::const_iterator hitr = intersections.cbegin();

    if (!hitr->nodePath.empty() && !hitr->nodePath.back()->getName().empty()) {
      mSelectedVisualizer = hitr->nodePath.back()->getName();
      //std::cout<<"Object identified by name "<<mSelectedVisualizer<<std::endl;
    } else if (hitr->drawable.valid()) {
      mSelectedVisualizer = hitr->drawable->className();
      //std::cout<<"Object identified by its drawable "<<mSelectedVisualizer<<std::endl;
    }
  }
}

/*!
 * \brief ViewerWidget::showVisualizerPickContextMenu
 * \param pos
 */
void ViewerWidget::showVisualizerPickContextMenu(const QPoint& pos)
{
  QString name = QString::fromStdString(mSelectedVisualizer);
  //std::cout<<"SHOW CONTEXT "<<name.toStdString()<<" compare "<<QString::compare(name,QString(""))<< std::endl;

  // The context widget
  QMenu contextMenu(tr("Context menu"), this);
  QMenu visualizerMenu(name, this);

  visualizerMenu.setIcon(QIcon(":/Resources/icons/animation.svg"));
  QAction action0(QIcon(":/Resources/icons/reset.svg"), tr("Reset Transparency and Texture"), this);
  QAction action1(QIcon(":/Resources/icons/transparency.svg"), tr("Change Transparency"), this);
  QAction action2(QIcon(":/Resources/icons/invisible.svg"), tr("Make Visualizer Invisible"), this);
  QAction action3(QIcon(":/Resources/icons/changeColor.svg"), tr("Change Color"), this);
  QAction action4(QIcon(":/Resources/icons/checkered.svg"), tr("Apply Checker Texture"), this);
  QAction action5(QIcon(":/Resources/icons/texture.svg"), tr("Apply Custom Texture"), this);
  QAction action6(QIcon(":/Resources/icons/undo.svg"), tr("Remove Texture"), this);

  connect(&action0, SIGNAL(triggered()), this, SLOT(resetTransparencyAndTextureForAllVisualizers()));
  connect(&action1, SIGNAL(triggered()), this, SLOT(changeVisualizerTransparency()));
  connect(&action2, SIGNAL(triggered()), this, SLOT(makeVisualizerInvisible()));
  connect(&action3, SIGNAL(triggered()), this, SLOT(changeVisualizerColor()));
  connect(&action4, SIGNAL(triggered()), this, SLOT(applyCheckerTexture()));
  connect(&action5, SIGNAL(triggered()), this, SLOT(applyCustomTexture()));
  connect(&action6, SIGNAL(triggered()), this, SLOT(removeTexture()));

  // If a visualizer is picked, one can change its properties
  AbstractVisualizerObject* visualizer = nullptr;
  if ((visualizer = mpAnimationWidget->getVisualization()->getBaseData()->getVisualizerObjectByID(mSelectedVisualizer))) {
    action2.setText(tr((std::string("Make ") + visualizer->getVisualizerType() + " Invisible").c_str()));
    contextMenu.addMenu(&visualizerMenu);
  }

  contextMenu.addAction(&action0);
  visualizerMenu.addAction(&action1);
  visualizerMenu.addAction(&action2);
  visualizerMenu.addSeparator();
  visualizerMenu.addAction(&action3);
  visualizerMenu.addSeparator();
  visualizerMenu.addAction(&action4);
  visualizerMenu.addAction(&action5);
  visualizerMenu.addAction(&action6);

  contextMenu.exec(this->mapToGlobal(pos));
}

/*!
 * \brief ViewerWidget::changeVisualizerTransparency
 * changes the transparency selection of a visualizer
 */
void ViewerWidget::changeVisualizerTransparency()
{
  AbstractVisualizerObject* visualizer = nullptr;
  if ((visualizer = mpAnimationWidget->getVisualization()->getBaseData()->getVisualizerObjectByID(mSelectedVisualizer))) {
    if (visualizer->isShape()) {
      ShapeObject* shape = static_cast<ShapeObject*>(visualizer);
      if (shape->_type.compare("dxf") == 0) {
        QString msg = tr("Transparency is not applicable for DXF-Files.");
        MessagesWidget::instance()->addGUIMessage(MessageItem(MessageItem::Modelica, msg,
                                                              Helper::scriptingKind, Helper::notificationLevel));
        mSelectedVisualizer = "";
        return;
      }
    }
    bool ok;
    const int min = 0, max = 100, step = 1; // Unit: [%]
    const int currentTransparency = visualizer->getTransparency() * (max - min) + min;
    const int transparency = QInputDialog::getInt(this, Helper::chooseTransparency, Helper::percentageLabel,
                                                  currentTransparency, min, max, step, &ok);
    if (ok) { // Picked transparency is not OK if the user cancels the dialog
      visualizer->setTransparency((float) (transparency - min) / (max - min));
      mpAnimationWidget->getVisualization()->modifyVisualizer(mSelectedVisualizer);
    }
    mSelectedVisualizer = "";
  }
}

/*!
 * \brief ViewerWidget::makeVisualizerInvisible
 * suppresses the visualization of this visualizer
 */
void ViewerWidget::makeVisualizerInvisible()
{
  AbstractVisualizerObject* visualizer = nullptr;
  if ((visualizer = mpAnimationWidget->getVisualization()->getBaseData()->getVisualizerObjectByID(mSelectedVisualizer))) {
    if (visualizer->isShape()) {
      ShapeObject* shape = static_cast<ShapeObject*>(visualizer);
      if (shape->_type.compare("dxf") == 0) {
        QString msg = tr("Invisibility is not applicable for DXF-Files.");
        MessagesWidget::instance()->addGUIMessage(MessageItem(MessageItem::Modelica, msg,
                                                              Helper::scriptingKind, Helper::notificationLevel));
        mSelectedVisualizer = "";
        return;
      }
    }
    visualizer->setTransparency(1.0);
    mpAnimationWidget->getVisualization()->modifyVisualizer(mSelectedVisualizer);
    mSelectedVisualizer = "";
  }
}

/*!
 * \brief ViewerWidget::changeVisualizerColor
 * opens a color dialog and selects a new color for the visualizer
 */
void ViewerWidget::changeVisualizerColor()
{
  AbstractVisualizerObject* visualizer = nullptr;
  if ((visualizer = mpAnimationWidget->getVisualization()->getBaseData()->getVisualizerObjectByID(mSelectedVisualizer))) {
    if (visualizer->isShape()) {
      ShapeObject* shape = static_cast<ShapeObject*>(visualizer);
      if (shape->_type.compare("dxf") == 0) {
        QString msg = tr("Changing the color is not applicable for DXF-Files.");
        MessagesWidget::instance()->addGUIMessage(MessageItem(MessageItem::Modelica, msg,
                                                              Helper::scriptingKind, Helper::notificationLevel));
        mSelectedVisualizer = "";
        return;
      }
    }
    const QColor currentColor = visualizer->getColor();
    const QColor color = QColorDialog::getColor(currentColor, this, Helper::chooseColor);
    if (color.isValid()) { // Picked color is invalid if the user cancels the dialog
      visualizer->setColor(color);
      mpAnimationWidget->getVisualization()->modifyVisualizer(mSelectedVisualizer);
    }
    mSelectedVisualizer = "";
  }
}

/*!
 * \brief ViewerWidget::applyCheckerTexture
 * adds a checkered texture to the visualizer
 */
void ViewerWidget::applyCheckerTexture()
{
  AbstractVisualizerObject* visualizer = nullptr;
  if ((visualizer = mpAnimationWidget->getVisualization()->getBaseData()->getVisualizerObjectByID(mSelectedVisualizer))) {
    if (visualizer->isShape()) {
      ShapeObject* shape = static_cast<ShapeObject*>(visualizer);
      if (shape->_type.compare("dxf") == 0 or shape->_type.compare("stl") == 0) {
        QString msg = tr("Texture feature is not applicable for CAD-Files.");
        MessagesWidget::instance()->addGUIMessage(MessageItem(MessageItem::Modelica, msg,
                                                              Helper::scriptingKind, Helper::notificationLevel));
        mSelectedVisualizer = "";
        return;
      }
    }
    visualizer->setTextureImagePath(":/Resources/bitmaps/check.png");
    mpAnimationWidget->getVisualization()->modifyVisualizer(mSelectedVisualizer);
    mSelectedVisualizer = "";
  }
}

/*!
 * \brief ViewerWidget::applyCustomTexture
 * adds a user-defined texture to the visualizer
 */
void ViewerWidget::applyCustomTexture()
{
  AbstractVisualizerObject* visualizer = nullptr;
  if ((visualizer = mpAnimationWidget->getVisualization()->getBaseData()->getVisualizerObjectByID(mSelectedVisualizer))) {
    if (visualizer->isShape()) {
      ShapeObject* shape = static_cast<ShapeObject*>(visualizer);
      if (shape->_type.compare("dxf") == 0 or shape->_type.compare("stl") == 0) {
        QString msg = tr("Texture feature is not applicable for CAD-Files.");
        MessagesWidget::instance()->addGUIMessage(MessageItem(MessageItem::Modelica, msg,
                                                              Helper::scriptingKind, Helper::notificationLevel));
        mSelectedVisualizer = "";
        return;
      }
    }
    const QString* currentFileName = nullptr; // File picker starts at the last open directory if any, otherwise the user's home directory
    const QString fileName = StringHandler::getOpenFileName(this, QString("%1 – %2").arg(Helper::applicationName).arg(Helper::chooseFile),
                                                            (QString*) currentFileName, Helper::bitmapFileTypes, nullptr);
    if (!fileName.isEmpty()) { // Picked file name is empty if the user cancels the dialog
      visualizer->setTextureImagePath(fileName.toStdString());
      mpAnimationWidget->getVisualization()->modifyVisualizer(mSelectedVisualizer);
    }
    mSelectedVisualizer = "";
  }
}

/*!
 * \brief ViewerWidget::removeTexture
 * removes the texture of the visualizer
 */
void ViewerWidget::removeTexture()
{
  AbstractVisualizerObject* visualizer = nullptr;
  if ((visualizer = mpAnimationWidget->getVisualization()->getBaseData()->getVisualizerObjectByID(mSelectedVisualizer))) {
    if (visualizer->isShape()) {
      ShapeObject* shape = static_cast<ShapeObject*>(visualizer);
      if (shape->_type.compare("dxf") == 0 or shape->_type.compare("stl") == 0) {
        QString msg = tr("Texture feature is not applicable for CAD-Files.");
        MessagesWidget::instance()->addGUIMessage(MessageItem(MessageItem::Modelica, msg,
                                                              Helper::scriptingKind, Helper::notificationLevel));
        mSelectedVisualizer = "";
        return;
      }
    }
    visualizer->setTextureImagePath("");
    mpAnimationWidget->getVisualization()->modifyVisualizer(mSelectedVisualizer);
    mSelectedVisualizer = "";
  }
}

/*!
 * \brief ViewerWidget::resetTransparencyAndTextureForAllVisualizers
 * sets all transparency and texture settings back to default
 */
void ViewerWidget::resetTransparencyAndTextureForAllVisualizers()
{
  std::vector<ShapeObject>& shapes = mpAnimationWidget->getVisualization()->getBaseData()->_shapes;
  std::vector<VectorObject>& vectors = mpAnimationWidget->getVisualization()->getBaseData()->_vectors;
  std::vector<std::reference_wrapper<AbstractVisualizerObject>> visualizers;
  visualizers.reserve(shapes.size() + vectors.size());
  for (ShapeObject& shape : shapes) {
    visualizers.push_back(shape);
  }
  for (VectorObject& vector : vectors) {
    visualizers.push_back(vector);
  }
  for (AbstractVisualizerObject& visualizer : visualizers) {
    visualizer.setTransparency(0.0);
    visualizer.setTextureImagePath("");
    mpAnimationWidget->getVisualization()->modifyVisualizer(visualizer._id);
  }
}

/*!
 * \brief ViewerWidget::mouseReleaseEvent
 * Passes the QWidget::mouseReleaseEvent() to Graphics Window.
 * \param event
 */
void ViewerWidget::mouseReleaseEvent(QMouseEvent *event)
{
  // 1 = left mouse button
  // 2 = middle mouse button
  // 3 = right mouse button
  unsigned int button = 0;
  switch (event->button()) {
    case Qt::LeftButton:
      button = 1;
      break;
    case Qt::MiddleButton:
      button = 2;
      break;
    case Qt::RightButton:
      button = 3;
      mSelectedVisualizer = "";
      break;
    default:
      break;
  }
  int pixelRatio = qCeil(qApp->devicePixelRatio());
  getEventQueue()->mouseButtonRelease(static_cast<float>(event->x() * pixelRatio), static_cast<float>(event->y() * pixelRatio), button);
}

/*!
 * \brief ViewerWidget::wheelEvent
 * Passes the QWidget::wheelEvent() to Graphics Window.
 * \param event
 */
void ViewerWidget::wheelEvent(QWheelEvent *event)
{
#if (QT_VERSION >= QT_VERSION_CHECK(5, 6, 0))
  static QPoint angleDelta = QPoint(0, 0);
  angleDelta += event->angleDelta();
  QPoint numDegrees = angleDelta / 8;
  QPoint numSteps = numDegrees / 15; // see QWheelEvent documentation
  if (numSteps.x() != 0 || numSteps.y() != 0) {
    angleDelta = QPoint(0, 0);
    osgGA::GUIEventAdapter::ScrollingMotion motion = (numSteps.x() > 0 || numSteps.y() > 0) ? osgGA::GUIEventAdapter::SCROLL_UP : osgGA::GUIEventAdapter::SCROLL_DOWN;
    getEventQueue()->mouseScroll(motion);
    event->accept();
  } else {
    event->ignore();
  }
#else // QT_VERSION_CHECK
  event->accept();
  int delta = event->delta();
  osgGA::GUIEventAdapter::ScrollingMotion motion = delta > 0 ? osgGA::GUIEventAdapter::SCROLL_UP : osgGA::GUIEventAdapter::SCROLL_DOWN;
  getEventQueue()->mouseScroll(motion);
#endif // QT_VERSION_CHECK
}

/*!
 * \brief ViewerWidget::event
 * Repaint the Graphics window on each user interaction.
 * \param event
 * \return
 */
bool ViewerWidget::event(QEvent *event)
{
  bool handled = GLWidget::event(event);
  // This ensures that the OSG widget is always going to be repainted after the
  // user performed some interaction. Doing this in the event handler ensures
  // that we don't forget about some event and prevents duplicate code.
  switch (event->type()) {
    case QEvent::KeyPress:
    case QEvent::KeyRelease:
    case QEvent::MouseButtonDblClick:
    case QEvent::MouseButtonPress:
    case QEvent::MouseButtonRelease:
    case QEvent::MouseMove:
    case QEvent::Wheel:
      update();
      break;
    default:
      break;
  }
  return handled;
}

/*!
 * \brief ViewerWidget::getEventQueue
 * \return
 */
osgGA::EventQueue* ViewerWidget::getEventQueue() const
{
  osgGA::EventQueue* eventQueue = mpGraphicsWindow->getEventQueue();
  assert (eventQueue != 0);
  return eventQueue;
}
