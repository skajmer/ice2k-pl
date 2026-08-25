#include <fx.h>
// #include <ice2k/comctl32.h>

#include "res/foxres.h"

FXIcon* mainIcon;

class HelloWindow : public FXMainWindow {
	FXDECLARE(HelloWindow);

protected:
	HelloWindow() {}

private:
	FXHorizontalFrame *contents;

public:
	long onCmdHello(FXObject*, FXSelector, void*);

public:
	enum {
		ID_MAINWIN = FXMainWindow::ID_LAST,
		ID_HELLO,
		ID_LAST
	};

public:
	HelloWindow(FXApp* a);

	virtual void create();
	virtual ~HelloWindow();
};

FXDEFMAP(HelloWindow) HelloWindowMap[] = {
	FXMAPFUNC(SEL_COMMAND,           HelloWindow::ID_HELLO,  HelloWindow::onCmdHello),
};

FXIMPLEMENT(HelloWindow, FXMainWindow, HelloWindowMap, ARRAYNUMBER(HelloWindowMap));

HelloWindow::HelloWindow(FXApp *a) : FXMainWindow(a, "Witaj świecie!", mainIcon, NULL, DECOR_ALL, 0,0,0,0) {
	new FXButton(this, "Witaj świecie!", NULL, this, ID_HELLO, BUTTON_NORMAL);
}

HelloWindow::~HelloWindow() {
}

void HelloWindow::create() {
	FXRootWindow* rootwin = getApp()->getRootWindow();
	puts( ((FXTopWindow*)rootwin->getFirst())->getTitle().text());
	FXMainWindow::create();

	show(PLACEMENT_SCREEN);
}

long HelloWindow::onCmdHello(FXObject*, FXSelector, void*) {
	puts("Witaj świecie!");
	return 1;
}

int main(int argc, char *argv[]) {
	FXApp application("Cześć", "I2KTest");
	mainIcon = new FXGIFIcon(&application, resico_mainicon, 0, IMAGE_OPAQUE);

	application.init(argc, argv);

	new HelloWindow(&application);

	application.create();
	return application.run();
}
