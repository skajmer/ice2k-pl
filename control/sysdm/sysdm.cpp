#include <fx.h>
#include <ice2k/comctl32.h>
#include <ice2k/branding.h>
#include "res/foxres.h"

#include <stdio.h>
#include <string.h>
#include <cpuid.h>
#include <sys/sysinfo.h>
#include <stdint.h>
#include <locale.h>
#include <unistd.h>
#include <limits.h>

#include <ifaddrs.h>
#include <net/if.h>
#include <netdb.h>
#include <errno.h>


#include <sys/types.h>
#include <pwd.h>
#include <ctype.h>
#include <sys/wait.h>
FXMainWindow* sysdmwin;


int getIPAddress(char* str) // taken from hostname utility, slightly modified
{                           // only returns 1 ipv4 address
	struct ifaddrs *ifa, *ifap;
	//char *p;
	char buf[NI_MAXHOST];
	int flags, ret, family, addrlen;

	flags = NI_NUMERICHOST;

	if (getifaddrs(&ifa) != 0) {
		fprintf(stderr, "%s", strerror(errno));
		return errno;
	}

	for (ifap = ifa; ifap != NULL; ifap = ifap->ifa_next) {
		/* Skip interfaces that have no configured addresses */
		if (ifap->ifa_addr == NULL)
			continue;

		/* Skip the loopback interface */
		if (ifap->ifa_flags & IFF_LOOPBACK)
			continue;

		/* Skip interfaces that are not UP */
		if (!(ifap->ifa_flags & IFF_UP))
			continue;

		/* Only handle IPv4 addresses */
		family = ifap->ifa_addr->sa_family;
		if (family != AF_INET)
			continue;

		addrlen = (family == AF_INET) ? sizeof(struct sockaddr_in) :
			sizeof(struct sockaddr_in6);

		ret = getnameinfo(ifap->ifa_addr, addrlen,
				buf, sizeof(buf), NULL, 0, flags);

		/* Just skip addresses that cannot be translated */
		if (ret != 0) {
			if (ret != EAI_NONAME) {
				fprintf(stderr, "%s", gai_strerror(ret));
				return ret;
			}
		} else {
			strcpy(str, buf);
			break;
		}
	}
	freeifaddrs(ifa);
	return 0;
}

// thank you https://en.wikipedia.org/wiki/CPUID#EAX=8000'0002h,8000'0003h,8000'0004h:_Processor_Brand_String
/*int getCpuString(char* output) {
#ifdef __x86_64__
#define _CPUID
#endif

#ifdef __i386__
#define _CPUID
#endif

#ifdef _CPUID
	unsigned int regs[12];      // i know i can use the linux apis for this...
								// but that's boring and i want to have fun :P
	__cpuid(0x80000000, regs[0], regs[1], regs[2], regs[3]);

	if (regs[0] < 0x80000004)
		return 1;

	__cpuid(0x80000002, regs[0], regs[1], regs[2], regs[3]);
	__cpuid(0x80000003, regs[4], regs[5], regs[6], regs[7]);
	__cpuid(0x80000004, regs[8], regs[9], regs[10], regs[11]);

	memcpy(output, regs, sizeof(regs));
	output[sizeof(regs)] = '\0';

	return 0;
#else
	return 1;
#endif
}*/

int getCpuString(char* output) {
	FILE* fp;
	char line[1024];

	output[0] = '\0';
	fp = fopen("/proc/cpuinfo", "r");

	if (fp == NULL) return 0;

	while(fgets(line, sizeof(line), fp)) {
		if (strncmp(line, "model name", sizeof("model name")-1) != 0) continue;
		line[strcspn(line, "\n")] = '\0';
		strcpy(output, line+13);
		break;
	}

	fclose(fp);

	return 1;
}

// https://stackoverflow.com/questions/22582989/word-wrap-program-c
inline int wordlen(const char * str){
	int tempindex=0;
	while (str[tempindex]!=' ' && str[tempindex]!=0 && str[tempindex]!='\n') {
		++tempindex;
	}
	return(tempindex);
}

void wrap(char * s, const int wrapline){
	int index = 0;
	int curlinelen = 0;

	while (s[index] != '\0') {
		if (s[index] == '\n')
			curlinelen = 0;

		else if (s[index] == ' ') {
			if (curlinelen+wordlen(&s[index+1]) >= wrapline) {
				s[index] = '\n';
				curlinelen = 0;
			}
		}

		curlinelen++;
		index++;
	}
}

void formatnum(long unsigned num, char *buffer) { // from ice2kver
	char temp[1024];
	sprintf(temp, "%lu", num / 1024);

	int len = strlen(temp);
	int commas = (len - 1) / 3;
	int new_len = len + commas;

	buffer[new_len] = '\0';
	int i = len - 1, j = new_len - 1;

	int count = 0;
	while (i >= 0) {
		if (count == 3) {
			buffer[j--] = ',';
			count = 0;
		}
		buffer[j--] = temp[i--];
		count++;
	}
}


class ChangeHostnameBox : public FXDialogBox {
	FXDECLARE(ChangeHostnameBox)

	private:
		// Controls
		FXHorizontalFrame *cont;                 // Container

		FXLabel           *icon;                 // About icon
		FXLabel           *text;                 // About text

		FXButton          *okbtn;                // OK button
		FXButton          *cancelbtn;            // Cancel button
												 //
		FXTextField       *compfield;

	protected:
		ChangeHostnameBox() {}

	public:

		// Message handlers
		long onSetFocus(FXObject*,FXSelector,void*);
		long onCmdAccept(FXObject*,FXSelector,void*);




	public:

		// Messages for our class
		enum {
			ID_ACCEPT=FXMainWindow::ID_LAST,
			ID_LAST
			//ID_SETFOCUS_T
		};

	public:

		// ChangeHostnameBox's constructor
		ChangeHostnameBox(FXWindow* owner);

		// Initialize
		virtual void create();

// hack to fix focusing issues on icewm
//
// fox toolkit calls setfocus on the main window one way or another
// and the way fox focuses the window causes issues and right now
// i'm too lazy to fork fox to fix it

		void setFocus() {};

		virtual ~ChangeHostnameBox();
};



// Change computer name window
FXDEFMAP(ChangeHostnameBox) ChangeHostnameBoxMap[] = {
	FXMAPFUNC(SEL_COMMAND, ChangeHostnameBox::ID_ACCEPT, ChangeHostnameBox::onCmdAccept),
};

FXIMPLEMENT(ChangeHostnameBox,FXDialogBox,ChangeHostnameBoxMap,ARRAYNUMBER(ChangeHostnameBoxMap));


long ChangeHostnameBox::onCmdAccept(FXObject* obj, FXSelector sel, void* ptr) {
	int status;
	char newhostname[HOST_NAME_MAX+1] = {0};
	
	strncpy(newhostname, compfield->getText().text(), sizeof(newhostname)-1);
	newhostname[sizeof(newhostname)-1] = '\0';

	if (fork() == 0) {
		char* cmd = (char*)"i2ksudo";
		char* args[] = { (char*)"i2ksudo", (char*)INSTPREFIX "/libexec/ice2k/sethostname", (char*)newhostname, (char*)NULL};
		execvp(cmd, args);
	} else {
		wait(&status);
		if (WIFEXITED(status)) {
			int exitcode = WEXITSTATUS(status);

			switch (exitcode) {
				case 0:
					FXMessageBox::information(this, MBOX_OK, "Identyfikacja sieciowa",
							"Musisz uruchomić ponownie ten komputer, aby zmiany zostały wprowadzone.");
					break;
				case 2:
					FXMessageBox::error(this, MBOX_OK, "Identyfikacja sieciowa",
							"Nie wpisano nazwy!");
					break;
				case 3:
					FXMessageBox::error(this, MBOX_OK, "Identyfikacja sieciowa",
							"Nowa nazwa jest nieprawidłowa!");
					break;
				default:
					FXMessageBox::error(this, MBOX_OK, "Identyfikacja sieciowa",
							"Wystąpił błąd przy zmienianiu nazwy.");	
			}
		} else {
			FXMessageBox::error(this, MBOX_OK, "Identyfikacja sieciowa",
				"Wystąpił błąd przy zmienianiu nazwy.");	
		}
	}

	FXDialogBox::onCmdAccept(obj, sel, ptr);

	return 1;
}

ChangeHostnameBox::ChangeHostnameBox(FXWindow* owner):

FXDialogBox(owner, "Zmiany identyfikacji", DECOR_TITLE|DECOR_BORDER|DECOR_CLOSE, 0, 0, 0, 0,
		11, 12, 11, 11, 0, 0) {
	new FXLabel(this, "Możesz zmienić nazwę i członkostwo tego komputera. musisz zainstalować", NULL, LABEL_NORMAL|JUSTIFY_LEFT, 0,0,0,0,  1,0,0,0);
	new FXLabel(this, "sieć przed zmianą członkostwa domeny tego", NULL, LABEL_NORMAL|JUSTIFY_LEFT, 0,0,0,0,  1,0,0,0);
	new FXLabel(this, "komputera.", NULL, LABEL_NORMAL|JUSTIFY_LEFT, 0,0,0,0,  1,0,0,0);
	new FXSeparator(this, SEPARATOR_NONE|LAYOUT_FIX_HEIGHT, 0,0,0,19);

	char hostname[HOST_NAME_MAX+1];
	gethostname(hostname, HOST_NAME_MAX+1);

	new FXLabel(this, "Pełna nazwa komputera:", NULL, LABEL_NORMAL|JUSTIFY_LEFT, 0,0,0,0,  1,0,0,2);

	compfield = new FXTextField(this, 49,NULL,0,FRAME_SUNKEN|FRAME_THICK);
	//compfield->setFocus();

	compfield->setText(hostname);
	compfield->selectAll();

	new FXSeparator(this, SEPARATOR_NONE|LAYOUT_FIX_HEIGHT, 0,0,0,86); // why so much whitespace?

	FXGroupBox* membergrp = new FXGroupBox(this, "Członkostwo", FRAME_THICK|LAYOUT_FILL_X, 0,0,0,0, 12,12,2,12);
	FXRadioButton* domainrad = new FXRadioButton(membergrp, "&Domena:", NULL, 0, RADIOBUTTON_NORMAL, 0,0,0,0,  2,0,2,0);
	domainrad->disable();
	FXTextField* domaintxt = new FXTextField(membergrp, 41, NULL, 0, LAYOUT_FIX_X|TEXTFIELD_NORMAL, 31,0,0,0,  2,2,2,1);
	domaintxt->disable();
	FXRadioButton* workgrad = new FXRadioButton(membergrp, "Grupa robocza:", NULL, 0, RADIOBUTTON_NORMAL, 0,0,0,0,  2,0,4,0);
	workgrad->setCheck(TRUE);
	workgrad->disable();
	FXTextField* worktxt = new FXTextField(membergrp, 41, NULL, 0, LAYOUT_FIX_X|TEXTFIELD_NORMAL, 31,0,0,0,  2,2,2,1);
	worktxt->setText("GRUPA_ROBOCZA");
	worktxt->setTextColor(getApp()->getShadowColor());
	worktxt->disable();
	membergrp->disable();


	new FXSeparator(this, SEPARATOR_NONE|LAYOUT_FIX_HEIGHT, 0,0,0,9);

	//membergrp->disable();

	FXHorizontalFrame* btncont = new FXHorizontalFrame(this, LAYOUT_RIGHT, 0, 0, 0, 0, 0, 0, 0, 0, 6, 0);

	okbtn = new FXButton(btncont, "OK", NULL, this, ID_ACCEPT,
			BUTTON_DEFAULT|BUTTON_INITIAL|FRAME_THICK|FRAME_RAISED|LAYOUT_FIX_WIDTH|LAYOUT_FIX_HEIGHT,
			0, 0, 75, 23, 3, 3, 2, 3);

	cancelbtn = new FXButton(btncont, "Anuluj", NULL, this, ID_CANCEL,
			BUTTON_DEFAULT|FRAME_THICK|FRAME_RAISED|LAYOUT_FIX_WIDTH|LAYOUT_FIX_HEIGHT,
			0, 0, 75, 23, 3, 3, 2, 3);
	//okbtn->setFocus();
}


ChangeHostnameBox::~ChangeHostnameBox() {
}

void ChangeHostnameBox::create() {
	FXDialogBox::create();
	compfield->setFocus();
}



// Main Window
class SystemPropertiesWindow : public FXMainWindow {

	// Macro for class hierarchy declarations
	FXDECLARE(SystemPropertiesWindow)

	private:
		FXVerticalFrame*          generalframe;
		FXVerticalFrame*          networkframe;
		FXVerticalFrame*          hardwareframe;
		FXVerticalFrame*          userframe;
		FXVerticalFrame*          advframe;

		FXHorizontalFrame*        btncont;
		FXTabBook*                tabbook;
		FXButton*                 okbtn;
		FXButton*                 cancelbtn;
		FXButton*                 applybtn;

		FXHorizontalFrame*        horcont;
		FXVerticalFrame*          vercont;



	protected:
		SystemPropertiesWindow(){}

	public:

		// Message handlers
		long onAccept(FXObject*,FXSelector,void*);
		long onChangeHostname(FXObject*,FXSelector,void*);
		long onCmdEnvVars(FXObject*,FXSelector,void*);
		long onCmdNtldr(FXObject*,FXSelector,void*);
		long onCmdDevmgmt(FXObject*,FXSelector,void*);
		long onTimeoutText(FXObject*,FXSelector,void*);


		FXText* makeWrapLabel(FXComposite* p, const FXString& text,
				FXuint opts=LAYOUT_FILL_X,
				FXint x=0, FXint y=0, FXint w=0, FXint h=0,
				FXint pl=0, FXint pr=0, FXint pt=0, FXint pb=12);



	public:

		// Messages for our class
		enum {
			ID_CHANGEHOSTNAME=FXMainWindow::ID_LAST,
			ID_ACCEPT,
			ID_CANCEL,
			ID_ENVVARS,
			ID_NTLDR,
			ID_DEVMGMT,
			ID_TEXT,
			ID_LAST,
		};

	public:

		// CtrlAltDelWindow's constructor
		SystemPropertiesWindow(FXApp* a);

		// Initialize
		virtual void create();
		void setFocus() {};

		virtual ~SystemPropertiesWindow();
};

FXDEFMAP(SystemPropertiesWindow) SystemPropertiesWindowMap[] = {
	FXMAPFUNC(SEL_COMMAND, SystemPropertiesWindow::ID_ACCEPT, SystemPropertiesWindow::onAccept),
	FXMAPFUNC(SEL_COMMAND, SystemPropertiesWindow::ID_CANCEL, SystemPropertiesWindow::onAccept),
	FXMAPFUNC(SEL_COMMAND, SystemPropertiesWindow::ID_CHANGEHOSTNAME, SystemPropertiesWindow::onChangeHostname),
	FXMAPFUNC(SEL_COMMAND, SystemPropertiesWindow::ID_ENVVARS, SystemPropertiesWindow::onCmdEnvVars),
	FXMAPFUNC(SEL_COMMAND, SystemPropertiesWindow::ID_NTLDR, SystemPropertiesWindow::onCmdNtldr),

	FXMAPFUNC(SEL_COMMAND, SystemPropertiesWindow::ID_DEVMGMT, SystemPropertiesWindow::onCmdDevmgmt),
	FXMAPFUNC(SEL_CHORE, SystemPropertiesWindow::ID_TEXT, SystemPropertiesWindow::onTimeoutText),

};

FXIMPLEMENT(SystemPropertiesWindow,FXMainWindow,SystemPropertiesWindowMap,ARRAYNUMBER(SystemPropertiesWindowMap))



	SystemPropertiesWindow::~SystemPropertiesWindow() {
	}

// ctrl+f, up, "ChangeHostnameBox::setFocus" and find next


#define MAXWRAP 8
int wraplabelscnt = 0;

FXText* wraplabels[MAXWRAP+1] = {NULL};

void SystemPropertiesWindow::create() {
	FXMainWindow::create();

	int i = 0;
	while (wraplabels[i] != NULL) {
		wraplabels[i]->setHeight(wraplabels[i]->getContentHeight());
		++i;
	}
}

FXText* SystemPropertiesWindow::makeWrapLabel(FXComposite* p, const FXString& text, FXuint opts,
		FXint x, FXint y, FXint w, FXint h,
		FXint pl, FXint pr, FXint pt, FXint pb) {
	if (wraplabelscnt+1 == MAXWRAP+1) {
		return NULL;
	}
	++wraplabelscnt;
	FXText* txt = new FXText(p, NULL, 0, opts|LAYOUT_FIX_HEIGHT|TEXT_WORDWRAP, x,y,w,h, pl,pr,pt,pb);
	txt->setMarginRight(0);
	txt->setMarginLeft(0);
	txt->setText(text);
	txt->setBackColor(getApp()->getBaseColor());
	txt->setDefaultCursor(getApp()->getDefaultCursor(DEF_ARROW_CURSOR));
	txt->disable();

	wraplabels[wraplabelscnt-1] = txt;

	//getApp()->addChore(this, ID_TEXT, txt);

	return txt;
}

long SystemPropertiesWindow::onAccept(FXObject* sender, FXSelector sel, void* ptr) {
	this->close();
	return 0;
}
long SystemPropertiesWindow::onTimeoutText(FXObject* sender, FXSelector sel, void* ptr) {
	FXText* txt = (FXText*)ptr;
	txt->setHeight(txt->getContentHeight());
	//puts("hi");
	return 1;
}
long SystemPropertiesWindow::onCmdDevmgmt(FXObject* sender, FXSelector sel, void* ptr) {
	system("devmgmt &");
	return 0;
}

long SystemPropertiesWindow::onCmdEnvVars(FXObject* sender, FXSelector sel, void* ptr) {


	system("xfw ~/.profile &"); // there is no good way of finding out where env vars are set on linux
	return 1;                   // and to globally set them because linux SUCKS
}

long SystemPropertiesWindow::onCmdNtldr(FXObject* sender, FXSelector sel, void* ptr) {
	//getApp()->beginWaitCursor();
	if (access("/boot/extlinux/extlinux.conf", F_OK) == 0)
		system("i2ksudox -E xfw /boot/extlinux/extlinux.conf &");
	else if (access("/boot/efi/loader/loader.conf", F_OK) == 0)
		system("i2ksudox -E xfw /boot/efi/loader/loader.conf &");
	else
		system("i2ksudox -E xfw /etc/default/grub &");

	/* if (access("/boot/extlinux/extlinux.conf", F_OK) == 0)
	   system("xfw /boot/extlinux/extlinux.conf &");
	   else if (access("/boot/efi/loader/loader.conf", F_OK) == 0)
	   system("xfw /boot/efi/loader/loader.conf &");
	   else
	   system("xfw /etc/default/grub &"); */

	//getApp()->endWaitCursor();
	return 1;
}

long SystemPropertiesWindow::onChangeHostname(FXObject* sender, FXSelector sel, void* ptr) {
	ChangeHostnameBox* hostnamebox = new ChangeHostnameBox(sysdmwin);
	hostnamebox->execute(PLACEMENT_OWNER);
	return 1;
}

int xp = 0;
int srv03 = 0;

//int main(int argc, char *argv[]) {
SystemPropertiesWindow::SystemPropertiesWindow(FXApp *app):FXMainWindow(app, "Właściwości systemu", NULL, NULL, DECOR_TITLE|DECOR_BORDER|DECOR_CLOSE, 0,0,404,436,  0,0,0,0,  0,0) {
	int blk_left_pad;
	this->changeFocus((FXWindow*)0);
	this->killFocus();
	char* windows = i2kBGetWinVersion();

	//FXText* frm;

	if ( !(strcmp(windows, "srv03")) ) {
		xp = 1;
		srv03 = 1;
	}

	if ( xp || (!(strcmp(windows, "xp"))) ) {
		xp = 1;
	}

	FXIcon* monitorimage;

	if (xp) {
		monitorimage = new FXGIFIcon(app, resico_monitorxp);
		blk_left_pad = 1;
	} else {
		monitorimage = new FXGIFIcon(app, resico_monitor);
		blk_left_pad = 0;
	}


	//application.init(argc, argv);
	//FXDialogBox *main=new FXDialogBox(app, "System Properties", NULL, NULL, DECOR_TITLE|DECOR_BORDER|DECOR_CLOSE|LAYOUT_FIX_WIDTH|LAYOUT_FIX_HEIGHT, 0,0,404,436,  0,0,0,0,  0,0);
	generalframe = new FXVerticalFrame(this,LAYOUT_FILL_X|LAYOUT_FILL_Y|FRAME_NONE, 0,0,0,0, 0,0,0,0, 0,0);
	tabbook = new FXTabBook(generalframe,NULL,0,LAYOUT_FILL_X|LAYOUT_FILL_Y|LAYOUT_RIGHT, 0,0,0,0, 6,6,7,6);


	btncont = new FXHorizontalFrame(generalframe, LAYOUT_RIGHT, 0, 0, 0, 0, 0, 6, 0, 7, 6, 0);

	okbtn = new FXButton(btncont, "OK", NULL, this, ID_ACCEPT, BUTTON_DEFAULT|BUTTON_NORMAL|LAYOUT_FIX_WIDTH|LAYOUT_FIX_HEIGHT, 0, 0, 75, 23, 0, 0, 0, 0);
	cancelbtn = new FXButton(btncont, "Anuluj", NULL, this, ID_CANCEL, BUTTON_NORMAL|BUTTON_DEFAULT|LAYOUT_FIX_WIDTH|LAYOUT_FIX_HEIGHT, 0, 0, 75, 23, 0, 0, 0, 0);
	applybtn = new FXButton(btncont, "&Zastosuj", NULL, this, 0, BUTTON_NORMAL|BUTTON_DEFAULT|LAYOUT_FIX_WIDTH|LAYOUT_FIX_HEIGHT, 0, 0, 75, 23, 0, 0, 0, 0);
	applybtn->disable();


	new FXTabItem(tabbook,"Ogólne",NULL,TAB_TOP_NORMAL,0,0,0,0,4,4,1,3);

	FXHorizontalFrame* horcont = new FXHorizontalFrame(tabbook,FRAME_THICK|FRAME_RAISED, 0,0,380,361, 24,24,15,16); 
	new FXLabel(horcont, "", monitorimage, LABEL_NORMAL, 0,0,0,0,  20,29,24,20);

	//new FXLabel(a, "", NULL,LAYOUT_FIX_X|LAYOUT_FIX_Y|LAYOUT_FIX_WIDTH|LAYOUT_FIX_HEIGHT, 0, 0, 380, 361);
	//new FXLabel(a, "a", NULL, LAYOUT_FILL_X|LAYOUT_FILL_Y,0, 0, 380, 361);
	//new FXLabel(a, "", monitoricon);
	vercont = new FXVerticalFrame(horcont,FRAME_NONE, 0,0,380,361, 2,2,2,2);
	new FXLabel(vercont, "System:",                    NULL, LABEL_NORMAL,              0,0,0,0,   0,0, 0,0);
	new FXLabel(vercont,    i2kBGetFullOSName(),       NULL, LABEL_NORMAL,              0,0,0,0,  18,0, 0,0);
	if (xp) {
#ifdef __x86_64__
		new FXLabel(vercont,    "Professional x64 Edition",               NULL, LABEL_NORMAL,              0,0,0,0,  18,0, 0,0);
		new FXLabel(vercont,    "Wersja 2003",               NULL, LABEL_NORMAL,              0,0,0,0,  18,0, 0,0);
		new FXLabel(vercont,    "Dodatek Service Pack 1",          NULL, LABEL_NORMAL,              0,0,0,0,  18,0, 0,0);
#else
		new FXLabel(vercont,    "Professional",               NULL, LABEL_NORMAL,              0,0,0,0,  18,0, 0,0);
		new FXLabel(vercont,    "Wersja 2002",               NULL, LABEL_NORMAL,              0,0,0,0,  18,0, 0,0);
		new FXLabel(vercont,    "Dodatek Service Pack 2",          NULL, LABEL_NORMAL,              0,0,0,0,  18,0, 0,0);
#endif
	} else {
		new FXLabel(vercont,    "5.00.2195",               NULL, LABEL_NORMAL,              0,0,0,0,  18,0, 0,0);
		new FXLabel(vercont,    "Service Pack 4",          NULL, LABEL_NORMAL,              0,0,0,0,  18,0, 0,0);
	}

	new FXSeparator(vercont, SEPARATOR_NONE|LAYOUT_FIX_HEIGHT, 0,0,0,4); // i could use a fxframe, but semantics r cute

	char hostname[HOST_NAME_MAX+1];
	gethostname(hostname, HOST_NAME_MAX+1);

	new FXLabel(vercont, "Zarejestrowano dla:",             NULL, LABEL_NORMAL,              0,0,0,0,  0,0, 0,0);
	new FXLabel(vercont,    getlogin(),                NULL, LABEL_NORMAL,              0,0,0,0, 18,0, 0,0);
	new FXLabel(vercont,    hostname,                  NULL, LABEL_NORMAL,              0,0,0,0, 18,0, 0,0);
	new FXLabel(vercont,    "76447-OEM-00119903-00102", NULL, LABEL_NORMAL,              0,0,0,0, 18,0, 0,0);

	//new FXLabel(vercont, " ", NULL, LABEL_NORMAL, 0,0,0,0,          0,0, 0,0);
	new FXSeparator(vercont, SEPARATOR_NONE|LAYOUT_FIX_HEIGHT, 0,0,0,13);

	new FXLabel(vercont, "Komputer:", NULL, LABEL_NORMAL, 0,0,0,0,          0,0, 0,0);
	//new FXLabel(vercont,    "Intel (R) Xeon(R) CPU", NULL, LABEL_NORMAL, 0,0,0,0,          18,0, 0,0);

	char cpubrand[1024];
	if (getCpuString(cpubrand)) {
		wrap(cpubrand, 28); //https://stackoverflow.com/questions/2351744/insert-line-breaks-in-long-string-word-wrap

		char* curLine = cpubrand;
		while (curLine) { // I LOVE STACK OVERFLOW https://stackoverflow.com/a/17983619
			char* nextLine = strchr(curLine, '\n');
			if (nextLine) *nextLine = '\0';
			new FXLabel(vercont, curLine,                  NULL, JUSTIFY_LEFT|LABEL_NORMAL, 0,0,0,0, 18,0, 0,0);    
			if (nextLine) *nextLine = '\n';
			curLine = nextLine ? (nextLine+1) : NULL;
		}
	} else {
		new FXLabel(vercont, "Nieznany",                  NULL, JUSTIFY_LEFT|LABEL_NORMAL, 0,0,0,0, 18,0, 0,0); // if you are on a toy arm cpu
	}

	new FXLabel(vercont,    "AT/AT COMPATIBLE", NULL, LABEL_NORMAL, 0,0,0,0,          18,0, 0,0);

	struct sysinfo sys_info; // from my own jawn ice2kver
							 // i should have probably just used sprintf...
	if( sysinfo(&sys_info) != 0)
		perror("sysinfo");

	char physmemtext[1024];
	if (xp) {
		formatnum(sys_info.totalram / 1024, physmemtext);
		strcat(physmemtext, " MB RAM");
	} else {
		formatnum(sys_info.totalram, physmemtext);
		strcat(physmemtext, " KB RAM");
	}

	new FXLabel(vercont, physmemtext, NULL, LABEL_NORMAL, 0,0,0,0,          18,0, 0,0);


	new FXTabItem(tabbook,"Identyfikacja sieciowa",NULL,TAB_TOP_NORMAL,0,0,0,0,4,4,1,3);
	networkframe = new FXVerticalFrame(tabbook,FRAME_THICK|FRAME_RAISED|LAYOUT_FILL_X|LAYOUT_FILL_Y);

	FXIcon* computericon;
	if (xp) {
		computericon = new FXBMPIcon(app, resico_compuzer_xp);
		computericon->blend(app->getBaseColor());
	} else {
		computericon = new FXGIFIcon(app, resico_compuzer);
	}

	FXHorizontalFrame* nettop = new FXHorizontalFrame(networkframe, LAYOUT_FILL_X, 0,0,0,0,   9,7,7,5,  17,16);

	new FXLabel(nettop, "", computericon, LABEL_NORMAL, 0,0,0,0,  0,0,0,0);

	new FXLabel(nettop, "System Windows używa następujących informacji do identyfikacji\n"
			"komputera w sieci.", NULL, LAYOUT_CENTER_Y|JUSTIFY_LEFT|LABEL_NORMAL, 0,0,0,0,  0,0,0,0);

	char hostnameup[HOST_NAME_MAX+1];

	memcpy(hostnameup, hostname, sizeof(hostnameup));

	char *upper = hostnameup;

	while (*upper) {
		*upper = toupper((unsigned char) *upper);
		upper++;
	}

	FXHorizontalFrame* compname = new FXHorizontalFrame(networkframe, LAYOUT_FILL_X, 0,0,0,0,   9,7,4,2,  0,0);
	new FXLabel(compname, "Pełna nazwa komputera:", NULL, JUSTIFY_LEFT|LABEL_NORMAL|LAYOUT_FIX_WIDTH,0,0,124,0, 0,0,0,0);
	new FXLabel(compname, hostnameup, NULL, LABEL_NORMAL,0,0,0,0, 0,0,0,0);

	FXHorizontalFrame* ipaddr = new FXHorizontalFrame(networkframe, LAYOUT_FILL_X, 0,0,0,0,   9,7,4,2,  0,0);
	new FXLabel(ipaddr, "Adres IPv4:", NULL, JUSTIFY_LEFT|LABEL_NORMAL|LAYOUT_FIX_WIDTH,0,0,124,0, 0,0,0,0);
	char ip[16];
	if (!getIPAddress(ip)) {
		new FXLabel(ipaddr, ip, NULL, LABEL_NORMAL,0,0,0,0, 0,0,0,0);
	} else {
		new FXLabel(ipaddr, "Nieznany", NULL, LABEL_NORMAL,0,0,0,0, 0,0,0,0);
	}

	FXHorizontalFrame* netidcont = new FXHorizontalFrame(networkframe, LAYOUT_FILL_X, 0,0,0,0,   9,7,15,4,  17,16);

	FXLabel* netidlbl = new FXLabel(netidcont, "Aby użyć kreatora identyfikacji sieciowej w celu\n"
			"przyłączenia się do domeny i utworzenia użytkownika\n"
			"lokalnego, kliknij przycisk Identyfikator sieciowy.",
			NULL, JUSTIFY_LEFT|LABEL_NORMAL|LAYOUT_FIX_WIDTH, 0,0,265,0,  0,0,0,0);

	netidlbl->disable();

	FXButton* netidbtn = new FXButton(netidcont, "Identyfikator...", NULL, NULL, 0, BUTTON_DEFAULT|BUTTON_NORMAL|LAYOUT_FIX_WIDTH|LAYOUT_FIX_HEIGHT, 0, 0, 80, 23, 0, 0, 0, 0);  

	netidbtn->disable();

	FXHorizontalFrame* renamecont = new FXHorizontalFrame(networkframe, LAYOUT_FILL_X, 0,0,0,0,   9,7,15,4,  17,16);

	new FXLabel(renamecont, "Aby zmienić nazwę tego komputera lub przyłączyć się\n"
			"do domeny, kliknij przycisk Właściwości.",
			NULL, JUSTIFY_LEFT|LABEL_NORMAL|LAYOUT_FIX_WIDTH, 0,0,265,0,  0,0,0,0);

	new FXButton(renamecont, "Właś&ciwości...", NULL, this, ID_CHANGEHOSTNAME, BUTTON_DEFAULT|BUTTON_NORMAL|LAYOUT_FIX_WIDTH|LAYOUT_FIX_HEIGHT, 0, 0, 80, 23, 0, 0, 0, 0);  



	new FXTabItem(tabbook,"Sprzęt",NULL,TAB_TOP_NORMAL,0,0,0,0,4,4,1,3);
	hardwareframe = new FXVerticalFrame(tabbook,FRAME_THICK|FRAME_RAISED, 0,0,0,0, 13,12,19,8, 0,0); 

	FXButton* btn;

	FXGroupBox* hdwwizgrp = new FXGroupBox(hardwareframe, "Kreator sprzętu", FRAME_THICK|LAYOUT_FILL_X, 0,0,0,0, 8,9,2,11, 0,0);
	FXIcon* hdwwizicon;
	if (xp) {
		hdwwizicon = new FXBMPIcon(app, resico_hdwwiz_xp);
		hdwwizicon->blend(app->getBaseColor());
	} else {
		hdwwizicon = new FXGIFIcon(app, resico_hdwwiz);
	}
	new FXLabel(hdwwizgrp, "", hdwwizicon, JUSTIFY_TOP|LABEL_NORMAL|LAYOUT_SIDE_LEFT|LAYOUT_FIX_HEIGHT, 0,0,0,70,  0,16,0,0);
	new FXLabel(hdwwizgrp, "Kreator sprzętu pomaga instalować, odinstalowywać,\n"
			"naprawiać, odłączać, wysuwać oraz zmieniać właściwości\n"
			"sprzętu.", NULL, JUSTIFY_LEFT|LABEL_NORMAL|LAYOUT_SIDE_TOP, 0,0,0,0,  0,0,0,0);


	btn = new FXButton(hdwwizgrp, "&Kreator sprzętu...", NULL, this, ID_CHANGEHOSTNAME, BUTTON_DEFAULT|BUTTON_NORMAL|LAYOUT_FIX_WIDTH|LAYOUT_FIX_HEIGHT|LAYOUT_BOTTOM|LAYOUT_SIDE_RIGHT, 0, 0, 147, 23, 0, 0, 0, 0);  
	btn->disable();


	new FXSeparator(hardwareframe, SEPARATOR_NONE|LAYOUT_FIX_HEIGHT, 0,0,0,5);

	FXGroupBox* devmgmtgrp = new FXGroupBox(hardwareframe, "Menedżer urządzeń", FRAME_THICK|LAYOUT_FILL_X, 0,0,0,0, 8,9,2,10, 7,0);
	FXIcon* devmgmticon = new FXGIFIcon(app, resico_devmgmt);
	new FXLabel(devmgmtgrp, "", devmgmticon, JUSTIFY_TOP|LABEL_NORMAL|LAYOUT_SIDE_LEFT|LAYOUT_FIX_HEIGHT, 0,0,0,76,  0,9,1,0);
	/* new FXLabel(devmgmtgrp, "The Device Manager lists all the hardware devices installed\n"
	   "on your computer. Use the Device Manager to change the\n"
	   "properties of any device.", NULL, JUSTIFY_LEFT|LABEL_NORMAL|LAYOUT_SIDE_TOP, 0,0,0,0,  0,0,0,0); */
	new FXLabel(devmgmtgrp, "Menedżer urządzeń wyświetla wszystkie urządzenia sprzętowe", NULL, JUSTIFY_LEFT|LABEL_NORMAL|LAYOUT_SIDE_TOP, 0,0,0,0,  0,0,0,0);
	new FXLabel(devmgmtgrp, "zainstalowane w komputerze. Możesz użyć Menedżera", NULL, JUSTIFY_LEFT|LABEL_NORMAL|LAYOUT_SIDE_TOP, 0,0,0,0,  0,0,0,0);
	new FXLabel(devmgmtgrp, "urządzeń, aby zmienić właściwości każdego urządzenia.", NULL, JUSTIFY_LEFT|LABEL_NORMAL|LAYOUT_SIDE_TOP, 0,0,0,0,  0,0,0,0);

	btn = new FXButton(devmgmtgrp, "&Menedżer urządzeń", NULL, this, ID_DEVMGMT, BUTTON_DEFAULT|BUTTON_NORMAL|LAYOUT_FIX_WIDTH|LAYOUT_FIX_HEIGHT|LAYOUT_BOTTOM|LAYOUT_SIDE_RIGHT, 0, 0, 147, 23, 0, 0, 0, 0);  
	btn = new FXButton(devmgmtgrp, "&Podpisywanie sterowników", NULL, this, ID_CHANGEHOSTNAME, BUTTON_DEFAULT|BUTTON_NORMAL|LAYOUT_FIX_WIDTH|LAYOUT_FIX_HEIGHT|LAYOUT_BOTTOM|LAYOUT_SIDE_RIGHT, 0, 0, 147, 23, 0, 0, 0, 0);  
	btn->disable();


	new FXSeparator(hardwareframe, SEPARATOR_NONE|LAYOUT_FIX_HEIGHT, 0,0,0,6);



	FXGroupBox* hwprofgrp = new FXGroupBox(hardwareframe, "Profile sprzętu", FRAME_THICK|LAYOUT_FILL_X, 0,0,0,0, 8,9,2,10, 0,0);
	FXIcon* hwproficon = new FXGIFIcon(app, resico_hwprof);
	new FXLabel(hwprofgrp, "", hwproficon, JUSTIFY_TOP|LABEL_NORMAL|LAYOUT_SIDE_LEFT|LAYOUT_FIX_HEIGHT, 0,0,0,71,  0,16,0,0);
	new FXLabel(hwprofgrp, "Dzięki profilom sprzętu możesz instalować oraz przechowywać\n"
			"różne konfiguracje sprzętu.", NULL, JUSTIFY_LEFT|LABEL_NORMAL|LAYOUT_SIDE_TOP, 0,0,0,0,  0,0,0,0);


	btn = new FXButton(hwprofgrp, "Profile &sprzętu", NULL, this, ID_CHANGEHOSTNAME, BUTTON_DEFAULT|BUTTON_NORMAL|LAYOUT_FIX_WIDTH|LAYOUT_FIX_HEIGHT|LAYOUT_BOTTOM|LAYOUT_SIDE_RIGHT, 0, 0, 147, 23, 0, 0, 0, 0);  
	btn->disable();


	new FXTabItem(tabbook,"Profile użytkownika",NULL,TAB_TOP_NORMAL,0,0,0,0,4,4,1,3);
	userframe = new FXVerticalFrame(tabbook,FRAME_THICK|FRAME_RAISED,0,0,0,0,  14,13,18,10, 0,0); 

	FXIcon* userproficon;
	if (xp) {
		userproficon = new FXBMPIcon(app, resico_userprof_xp);
		userproficon->blend(app->getBaseColor());
	} else {
		userproficon = new FXGIFIcon(app, resico_userprof);
	}


	FXPacker* infocont = new FXPacker(userframe,FRAME_NONE,0,0,0,0,  0,0,0,25, 26,0);
	new FXLabel(infocont, "", userproficon, JUSTIFY_TOP|LABEL_NORMAL|LAYOUT_SIDE_LEFT, 0,0,0,0,  0,0,0,0);
	new FXLabel(infocont, "Profile użytkownika przechowują ustawienia pulpitu oraz inne", NULL, JUSTIFY_TOP|LABEL_NORMAL|LAYOUT_SIDE_TOP, 0,0,0,0,  0,0,0,0);
	new FXLabel(infocont, "informacje związane z kontem użytkownika. Możesz utworzyć", NULL, JUSTIFY_TOP|LABEL_NORMAL|LAYOUT_SIDE_TOP, 0,0,0,0,  0,0,0,0);
	new FXLabel(infocont, "inny profil na każdym z używanych komputerów lub wybrać profil", NULL, JUSTIFY_TOP|LABEL_NORMAL|LAYOUT_SIDE_TOP, 0,0,0,0,  0,0,0,0);
	new FXLabel(infocont, "mobilny, który jest taki sam dla każdego z używanych komputerów.", NULL, JUSTIFY_TOP|LABEL_NORMAL|LAYOUT_SIDE_TOP, 0,0,0,0,  0,0,0,0);

	new FXLabel(userframe, "&Profile przechowywane na tym komputerze:", NULL, JUSTIFY_TOP|LABEL_NORMAL|LAYOUT_SIDE_TOP, 0,0,0,0,  0,0,0,4);

	FXPacker* tablecont = new FXPacker(userframe,LAYOUT_FILL_X|LAYOUT_FILL_Y,0,0,0,0,  0,1,0,0, 0,0);  
	FXPacker* tablecont2 = new FXPacker(tablecont,LAYOUT_FILL_X|LAYOUT_FILL_Y|FRAME_THICK|FRAME_SUNKEN,0,0,0,0,  0,0,0,0, 0,0);  
	FXTable* table=new FXTable(tablecont2,NULL,0,TABLE_READONLY|LAYOUT_FILL_X|LAYOUT_FILL_Y,0,0,0,0, 2,2,2,2);
	//table->setSelMode(SELECTTABLE_ROWS);

	FXFont* deffont = getApp()->getNormalFont();

	int res = getApp()->reg().readUnsignedEntry("SETTINGS","screenres",100); // since we cant get the actual font size, we have to calculate it ourselves

	float fontpxf = (float)deffont->getSize() / (float)10 * (float)res / (float)72;
	int fontpx = roundf(fontpxf);

	table->setCellBorderWidth(0);
	table->setRowHeaderWidth(0);
	table->setTableSize(0, 3);
	table->showHorzGrid(FALSE);
	table->showVertGrid(FALSE);

	struct passwd *user;

	unsigned int nobodyuid = getpwnam("nobody")->pw_uid;
	// https://stackoverflow.com/questions/14970938/print-out-all-users-of-a-machine-in-c
	int users = 0;

	table->setColumnText(0, "Nazwa");
	table->setColumnText(1, "UID");
	table->setColumnText(2, "Typ");	

	table->setDefRowHeight(fontpx + 3);

	table->setMarginLeft(4);
	table->getColumnHeader()->setPadTop(0);
	table->getColumnHeader()->setPadBottom(0);


	char passwdname[LOGIN_NAME_MAX+1];
	char uidstr[6]; // 16 bit integer = 5 characters + 1 for null byte

	while ( (user = getpwent() )) {
		if ( (user->pw_uid >= 1000 || user->pw_uid == 0 ) && !(user->pw_uid == nobodyuid)) {
			//printf("name: %d: %s\n", user->pw_uid, user->pw_name);
			table->insertRows(users,1);
			sprintf(passwdname, "%s\\%s", hostnameup, user->pw_name);
			table->setItemJustify(users, 0, FXTableItem::LEFT);
			table->setItemText(users, 0, passwdname);

			sprintf(uidstr, "%d", user->pw_uid);
			table->setItemText(users, 1, uidstr);

			table->setItemJustify(users, 2, FXTableItem::LEFT);
			table->setItemText(users, 2, "Lokalny");
			users++;

		}
	}

	table->setColumnWidth(0, 215);
	table->setColumnWidth(1, 50);
	table->setColumnWidth(2, 75);

	//FXHorizontalFrame* renamecont = new FXHorizontalFrame(userframe, LAYOUT_FILL_X, 0,0,0,0, 0,0,0,0,  17,16);
	FXMatrix* botbuttons = new FXMatrix(userframe, 3, LAYOUT_FILL_COLUMN|LAYOUT_FILL_X|MATRIX_BY_COLUMNS|PACK_UNIFORM_WIDTH|PACK_UNIFORM_HEIGHT, 0,0,0,0,  0,0,9,0,  12,0);
	//botbuttons->setBackColor(FXRGB(255,0,0));


	btn = new FXButton(botbuttons, "&Usuń", NULL, NULL, 0, BUTTON_DEFAULT|BUTTON_NORMAL|LAYOUT_FIX_HEIGHT|LAYOUT_FILL_COLUMN|LAYOUT_FILL_X,         0,0,0,21, 0,0,0,0);
	btn->disable();
	btn = new FXButton(botbuttons, "&Zmień typ...", NULL, NULL, 0, BUTTON_DEFAULT|BUTTON_NORMAL|LAYOUT_FIX_HEIGHT|LAYOUT_FILL_COLUMN|LAYOUT_FILL_X, 0,0,0,21, 0,0,0,0);  
	btn->disable();
	btn = new FXButton(botbuttons, "&Kopiuj do...", NULL, NULL, 0, BUTTON_DEFAULT|BUTTON_NORMAL|LAYOUT_FIX_HEIGHT|LAYOUT_FILL_COLUMN|LAYOUT_FILL_X,     0,0,0,21, 0,0,0,0);  
	btn->disable();


	new FXTabItem(tabbook,"Zaawansowane",NULL,TAB_TOP_NORMAL,0,0,0,0,4,4,1,3);
	advframe = new FXVerticalFrame(tabbook,FRAME_THICK|FRAME_RAISED, 0,0,0,0, 13,12,19,8, 0,0); 

	FXGroupBox* performgrp = new FXGroupBox(advframe, "Wydajność", FRAME_THICK|LAYOUT_FILL_X, 0,0,0,0, 8,9,2,11, 0,0);
	FXIcon* performicon = new FXGIFIcon(app, resico_perform);
	if (!xp) new FXLabel(performgrp, "", performicon, JUSTIFY_TOP|LABEL_NORMAL|LAYOUT_SIDE_LEFT|LAYOUT_FIX_HEIGHT, 0,0,0,70,  0,16,0,0);
	/*new FXLabel(performgrp, "Performance options control how applications use memory,\n"
			"which affects the speed of your computer.", NULL, JUSTIFY_LEFT|LABEL_NORMAL|LAYOUT_SIDE_TOP, 0,0,0,0,  0,0,0,0);*/

	makeWrapLabel(performgrp, "Efekty wizualne, planowanie użycia procesora, wykorzystanie "
			"pamięci i pamięć wirtualna.", LAYOUT_FILL_X,
			blk_left_pad);
	btn = new FXButton(performgrp, "&Ustawienia", NULL, NULL, 0, BUTTON_DEFAULT|BUTTON_NORMAL|LAYOUT_FIX_WIDTH|LAYOUT_FIX_HEIGHT|LAYOUT_BOTTOM|LAYOUT_SIDE_RIGHT, 0, 0, 147, 23, 0, 0, 0, 0);  
	btn->disable();


	new FXSeparator(advframe, SEPARATOR_NONE|LAYOUT_FIX_HEIGHT, 0,0,0,5);

	FXGroupBox* envvarsgrp = new FXGroupBox(advframe, "Zmienne środowiskowe", FRAME_THICK|LAYOUT_FILL_X, 0,0,0,0, 8,9,2,10, 7,0);
	FXIcon* envvarsicon = new FXGIFIcon(app, resico_envvars);
	if (!xp) new FXLabel(envvarsgrp, "", envvarsicon, JUSTIFY_TOP|LABEL_NORMAL|LAYOUT_SIDE_LEFT|LAYOUT_FIX_HEIGHT, 0,0,0,71,  0,9,1,0);
	/* new FXLabel(devmgmtgrp, "The Device Manager lists all the hardware devices installed\n"
	   "on your computer. Use the Device Manager to change the\n"
	   "properties of any device.", NULL, JUSTIFY_LEFT|LABEL_NORMAL|LAYOUT_SIDE_TOP, 0,0,0,0,  0,0,0,0); */
	//new FXLabel(envvarsgrp, "Environment variables tell your computer where to find", NULL, JUSTIFY_LEFT|LABEL_NORMAL|LAYOUT_SIDE_TOP, 0,0,0,0,  0,0,0,0);
	//new FXLabel(envvarsgrp, "certain types of information.", NULL, JUSTIFY_LEFT|LABEL_NORMAL|LAYOUT_SIDE_TOP, 0,0,0,0,  0,0,0,0);
	makeWrapLabel(envvarsgrp, "Zmienne środowiskowe informują komputer, gdzie można "
			"znaleźć określone informacje.", LAYOUT_FILL_X,
			blk_left_pad);

	btn = new FXButton(envvarsgrp, "Z&mienne środowiskowe", NULL, this, ID_ENVVARS, BUTTON_DEFAULT|BUTTON_NORMAL|LAYOUT_FIX_WIDTH|LAYOUT_FIX_HEIGHT|LAYOUT_BOTTOM|LAYOUT_SIDE_RIGHT, 0, 0, 147, 23, 0, 0, 0, 0);  

	new FXSeparator(advframe, SEPARATOR_NONE|LAYOUT_FIX_HEIGHT, 0,0,0,6);



	FXGroupBox* ntldrgrp = new FXGroupBox(advframe, "Uruchamianie i odzyskiwanie", FRAME_THICK|LAYOUT_FILL_X, 0,0,0,0, 8,9,2,10, 0,0);
	FXIcon* ntldricon = new FXGIFIcon(app, resico_ntldr);
	if (!xp) new FXLabel(ntldrgrp, "", ntldricon, JUSTIFY_TOP|LABEL_NORMAL|LAYOUT_SIDE_LEFT|LAYOUT_FIX_HEIGHT, 0,0,0,76,  0,16,0,0);
	/*new FXLabel(ntldrgrp, "Startup and recovery options tell your computer how to start\n"
			"and what to do if an error causes your computer to stop.", NULL, JUSTIFY_LEFT|LABEL_NORMAL|LAYOUT_SIDE_TOP, 0,0,0,0,  0,0,0,0);*/
	makeWrapLabel(ntldrgrp, "Informacje o uruchamianiu systemu, awariach systemu i\n"
			"debugowaniu.", LAYOUT_FILL_X,
			blk_left_pad);

	btn = new FXButton(ntldrgrp, "Ust&awienia", NULL, this, ID_NTLDR, BUTTON_DEFAULT|BUTTON_NORMAL|LAYOUT_FIX_WIDTH|LAYOUT_FIX_HEIGHT|LAYOUT_BOTTOM|LAYOUT_SIDE_RIGHT, 0, 0, 147, 23, 0, 0, 0, 0);  

	//getApp()->addTimeout(this,SystemPropertiesWindow::ID_SETFOCUS_T,10);

}


int main(int argc,char *argv[]) {
	FXApp application("sysdm", "Ice2KProj");

	application.init(argc,argv);
	sysdmwin = new SystemPropertiesWindow(&application);

	application.create();

	sysdmwin->show(PLACEMENT_OWNER);
	return application.run();
}

