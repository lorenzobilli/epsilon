#include "histogram_controller.h"
#include <assert.h>
#include <math.h>

namespace Statistics {

HistogramController::HistogramController(Responder * parentResponder, HeaderViewController * headerViewController, Store * store) :
  ViewController(parentResponder),
  HeaderViewDelegate(headerViewController),
  m_bannerView(HistogramBannerView()),
  m_view(HistogramView(store, &m_bannerView)),
  m_settingButton(Button(this, "Reglages de l'histogramme",Invocation([](void * context, void * sender) {
    HistogramController * histogramController = (HistogramController *) context;
    StackViewController * stack = ((StackViewController *)histogramController->stackController());
    stack->push(histogramController->histogramParameterController());
  }, this))),
  m_store(store),
  m_histogramParameterController(nullptr, store)
{
}

const char * HistogramController::title() const {
  return "Histogramme";
}

View * HistogramController::view() {
  return &m_view;
}

StackViewController * HistogramController::stackController() {
  StackViewController * stack = (StackViewController *)(parentResponder()->parentResponder()->parentResponder());
  return stack;
}

HistogramParameterController * HistogramController::histogramParameterController() {
  return &m_histogramParameterController;
}

bool HistogramController::handleEvent(Ion::Events::Event event) {
  if (event == Ion::Events::Down) {
    if (!m_view.isMainViewSelected()) {
      headerViewController()->setSelectedButton(-1);
      m_view.selectMainView(true);
      m_view.reloadSelection();
      reloadBannerView();
      return true;
    }
    return false;
  }
  if (event == Ion::Events::Up) {
    if (!m_view.isMainViewSelected()) {
      headerViewController()->setSelectedButton(-1);
      app()->setFirstResponder(tabController());
      return true;
    }
    m_view.selectMainView(false);
    headerViewController()->setSelectedButton(0);
    return true;
  }
  if (m_view.isMainViewSelected() && (event == Ion::Events::Left || event == Ion::Events::Right)) {
    int direction = event == Ion::Events::Left ? -1 : 1;
    m_view.reloadSelection();
    if (m_store->selectNextBarToward(direction)) {
      m_view.reload();
    } else {
      m_view.reloadSelection();
    }
    reloadBannerView();
    return true;
  }
  return false;
}

void HistogramController::didBecomeFirstResponder() {
  uint32_t storeChecksum = m_store->checksum();
  if (m_storeVersion != storeChecksum) {
    m_storeVersion = storeChecksum;
    m_store->initBarParameters();
    m_store->initWindowParameters();
  }
  headerViewController()->setSelectedButton(-1);
  m_view.selectMainView(true);
  m_view.reload();
  reloadBannerView();
}

int HistogramController::numberOfButtons() const {
  return 1;
}
Button * HistogramController::buttonAtIndex(int index) {
  return &m_settingButton;
}

bool HistogramController::isEmpty() {
  if (m_store->sumOfColumn(1) == 0) {
    return true;
  }
  return false;
}

const char * HistogramController::emptyMessage() {
  return "Aucune donnee a tracer";
}

Responder * HistogramController::defaultController() {
  return tabController();
}

Responder * HistogramController::tabController() const {
  return (parentResponder()->parentResponder()->parentResponder()->parentResponder());
}

void HistogramController::reloadBannerView() {
  char buffer[k_maxNumberOfCharacters+ Constant::FloatBufferSizeInScientificMode*2];
  const char * legend = "Interval [";
  int legendLength = strlen(legend);
  strlcpy(buffer, legend, legendLength+1);
  float lowerBound = m_store->selectedBar() - m_store->barWidth()/2;
  int lowerBoundNumberOfChar = Float(lowerBound).convertFloatToText(buffer+legendLength, Constant::FloatBufferSizeInScientificMode, Constant::NumberOfDigitsInMantissaInScientificMode);
  buffer[legendLength+lowerBoundNumberOfChar] = ';';
  float upperBound = m_store->selectedBar() + m_store->barWidth()/2;
  int upperBoundNumberOfChar = Float(upperBound).convertFloatToText(buffer+legendLength+lowerBoundNumberOfChar+1, Constant::FloatBufferSizeInScientificMode, Constant::NumberOfDigitsInMantissaInScientificMode);
  buffer[legendLength+lowerBoundNumberOfChar+upperBoundNumberOfChar+1] = '[';
  buffer[legendLength+lowerBoundNumberOfChar+upperBoundNumberOfChar+2] = 0;
  m_bannerView.setLegendAtIndex(buffer, 0);

  legend = "Effectif: ";
  legendLength = strlen(legend);
  strlcpy(buffer, legend, legendLength+1);
  float size = m_store->heightForBarAtValue(m_store->selectedBar());
  Float(size).convertFloatToText(buffer+legendLength, Constant::FloatBufferSizeInScientificMode, Constant::NumberOfDigitsInMantissaInScientificMode);
  m_bannerView.setLegendAtIndex(buffer, 1);

  legend = "Frequence: ";
  legendLength = strlen(legend);
  strlcpy(buffer, legend, legendLength+1);
  float frequency = size/m_store->sumOfColumn(1);
  Float(frequency).convertFloatToText(buffer+legendLength, Constant::FloatBufferSizeInScientificMode, Constant::NumberOfDigitsInMantissaInScientificMode);
  m_bannerView.setLegendAtIndex(buffer, 2);
  m_bannerView.layoutSubviews();
}

}
