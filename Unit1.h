//---------------------------------------------------------------------------

#ifndef Unit1H
#define Unit1H
//---------------------------------------------------------------------------
#include <Classes.hpp>
#include <Controls.hpp>
#include <StdCtrls.hpp>
#include <Forms.hpp>
#include <ComCtrls.hpp>
#include <ExtCtrls.hpp>
#include <Dialogs.hpp>
#include <Menus.hpp>
#include <ImgList.hpp>
//---------------------------------------------------------------------------
class TTaskInfo
{
public:
        AnsiString LocalFile, RemoteFile;
};
//---------------------------------------------------------------------------
class TMyThread : public TThread
{
private:
        AnsiString LogText;
        AnsiString Task;
        AnsiString RemoteFile, LocalFile;
        AnsiString File;
        int PPos1, PMax1, PPos2, PMax2;

        AnsiString Port;
        HANDLE hCom;
        AnsiString atcmd;
        int BaudRate, FrameSize;

        TStringList *LogDebug;
        void __fastcall LogDebugAdd(AnsiString s, unsigned char buf[], int len);

        void __fastcall SyncLogAdd();
        void __fastcall LogAdd(AnsiString text);
        void __fastcall SyncGetTask();
        void __fastcall SyncDelTask();
        void __fastcall SyncListFileClear();
        void __fastcall SyncListFileAdd();
        void __fastcall ListFileAdd(AnsiString file);
        void __fastcall SyncListFileAddDir();
        void __fastcall ListFileAddDir(AnsiString dir);
        void __fastcall SyncProgress1();
        void __fastcall Progress1(int pos, int max = -1);
        void __fastcall SyncProgress2();
        void __fastcall Progress2(int pos, int max = -1);

        bool __fastcall SetState(HANDLE h, AnsiString state);
        bool __fastcall WriteBuf(HANDLE h, unsigned char buf[], int len);
        bool __fastcall ReadBuf(HANDLE h, unsigned char buf[], int len);
        bool __fastcall Wait(HANDLE h, unsigned char data[], int len);
        bool __fastcall Command(HANDLE h, unsigned short cmd,
                unsigned char data[], int len, unsigned char cont = 0x03);
        bool __fastcall Send(HANDLE h, unsigned char data[], int len, unsigned char cont = 0x03);
        int __fastcall Receive(HANDLE h, unsigned char buf[]);
public:
        bool Debug;
        void __fastcall Execute();

        unsigned int TimeOut;
        unsigned int WaitFirst;
        unsigned int WaitAT;
        unsigned int WaitBaud;
        unsigned int WaitCmd;
        unsigned int WaitTask;
        int RI, RM, RC, WM, WC;
        
        __fastcall TMyThread(bool CreateSuspended, AnsiString port, AnsiString at = "", int baud = 115200, int frame = 0x400);
        __fastcall ~TMyThread();
};
//---------------------------------------------------------------------------
class TForm1 : public TForm
{
__published:	// IDE 管理のコンポーネント
        TPanel *PanelTop;
        TButton *ButtonConnect;
        TComboBox *ComboCable;
        TComboBox *ComboPort;
        TPanel *PanelMain;
        TRichEdit *Log;
        TPanel *PanelBottom;
        TSplitter *Splitter2;
        TListBox *ListTask;
        TSplitter *Splitter1;
        TButton *ButtonClear;
        TComboBox *ComboCmd;
        TButton *ButtonAdd;
        TOpenDialog *OpenDialog;
        TEdit *EditFrame;
        TEdit *EditBaud;
        TPopupMenu *PopupMenu1;
        TMenuItem *KeyGetFile;
        TMenuItem *KeyDelFile;
        TComboBox *ComboMobile;
        TButton *ButtonDown;
        TButton *ButtonDel;
        TButton *ButtonUp;
        TProgressBar *ProgressBar1;
        TLabel *LabelProgress;
        TProgressBar *ProgressBar2;
        TListView *ListFile;
        TSaveDialog *SaveDialog;
        TImageList *IconListSmall;
        void __fastcall ButtonConnectClick(TObject *Sender);
        void __fastcall FormCreate(TObject *Sender);
        void __fastcall FormDestroy(TObject *Sender);
        void __fastcall LogChange(TObject *Sender);
        void __fastcall PanelTopResize(TObject *Sender);
        void __fastcall ButtonClearClick(TObject *Sender);
        void __fastcall ButtonAddClick(TObject *Sender);
        void __fastcall ButtonDelClick(TObject *Sender);
        void __fastcall ButtonUpClick(TObject *Sender);
        void __fastcall ButtonDownClick(TObject *Sender);
        void __fastcall PanelBottomResize(TObject *Sender);
        void __fastcall FormCloseQuery(TObject *Sender, bool &CanClose);
        void __fastcall KeyGetFileClick(TObject *Sender);
        void __fastcall KeyDelFileClick(TObject *Sender);
        void __fastcall ListFileDblClick(TObject *Sender);
        void __fastcall ListTaskDblClick(TObject *Sender);
        void __fastcall ComboMobileChange(TObject *Sender);
        void __fastcall ListFileColumnClick(TObject *Sender,
          TListColumn *Column);
private:	// ユーザー宣言
        AnsiString IniFile;
        bool Debug;
        int w1, wat, wbaud, wcmd, wtask, tout;
        int RI, RM, RC, WM, WC;
        TMyThread *MyThread;
        void __fastcall ThreadTerminate(TObject *Sender);
protected:
	void __fastcall WMDropFiles(TWMDropFiles &Msg);

#pragma warn -8027
        BEGIN_MESSAGE_MAP
        VCL_MESSAGE_HANDLER(WM_DROPFILES, TWMDropFiles, WMDropFiles)
        END_MESSAGE_MAP(TForm)
#pragma warn .8027
public:		// ユーザー宣言
        __fastcall TForm1(TComponent* Owner);
};
//---------------------------------------------------------------------------
extern PACKAGE TForm1 *Form1;
//---------------------------------------------------------------------------
#endif
