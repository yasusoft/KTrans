//---------------------------------------------------------------------------

#include <vcl.h>
#pragma hdrstop

#include "Unit1.h"
#include <IniFiles.hpp>
#define ShlObjHPP
#include <windowsx.h>
#include <shlobj.h>
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"
TForm1 *Form1;
//---------------------------------------------------------------------------
__fastcall TMyThread::TMyThread(bool CreateSuspended, AnsiString port, AnsiString at, int baud, int frame)
        : TThread(CreateSuspended)
{
  Port = port;
  hCom = INVALID_HANDLE_VALUE;
  atcmd = at;
  BaudRate = baud;
  FrameSize = frame;

  if (frame < 4096)
    TimeOut = 2500;
  else
    TimeOut = frame;
  WaitFirst = 500;
  WaitAT = 500;
  WaitBaud = 500;
  WaitCmd = 500;
  WaitTask = 100;

  /*
  RI = 500;
  RM = 10;
  RC = 5000;
  WM = 100;
  WC = 5000;
  */
  RI = 0;
  RM = 0;
  RC = 100;
  WM = 5000;
  WC = 5000;

  Debug = false;
  LogDebug = new TStringList;
}
//---------------------------------------------------------------------------
__fastcall TMyThread::~TMyThread()
{
  if (hCom != INVALID_HANDLE_VALUE)
    CloseHandle(hCom);

  if (Debug)
  {
    try {
      LogDebug->SaveToFile(ChangeFileExt(Application->ExeName, ".log"));
    } catch (...) { }
  }
}
//---------------------------------------------------------------------------
void __fastcall TMyThread::LogDebugAdd(AnsiString s, unsigned char buf[], int len)
{
  if (Debug)
  {
    if (len > 0)
    {
      AnsiString text = IntToStr(GetTickCount()) + "(" + s + "," + IntToStr(len) + ")\t:";
      for (int i = 0; i < len; i ++)
        text += " 0x" + IntToHex(buf[i], 2);
      LogDebug->Add(text);
    }
    else
      LogDebug->Add(IntToStr(GetTickCount()) + "(M)\t: " + s);
  }
}
//---------------------------------------------------------------------------
void __fastcall TMyThread::SyncLogAdd()
{
  Form1->Log->Lines->Add(LogText);
}
//---------------------------------------------------------------------------
void __fastcall TMyThread::LogAdd(AnsiString text)
{
  LogText = text;
  Synchronize(SyncLogAdd);
}
//---------------------------------------------------------------------------
void __fastcall TMyThread::SyncGetTask()
{
  if (Form1->ListTask->Items->Count > 0)
  {
    Task = Form1->ListTask->Items->Strings[0];
    TTaskInfo *task = (TTaskInfo*)Form1->ListTask->Items->Objects[0];
    if (task)
    {
      RemoteFile = task->RemoteFile;
      LocalFile = task->LocalFile;
    }
  }
  else
  {
    Task = "";
    RemoteFile = "";
    LocalFile = "";
  }
}
//---------------------------------------------------------------------------
void __fastcall TMyThread::SyncDelTask()
{
  if (Form1->ListTask->Items->Count > 0)
  {
    if (Form1->ListTask->Items->Objects[0])
      delete (TTaskInfo*)Form1->ListTask->Items->Objects[0];
    Form1->ListTask->Items->Delete(0);
  }
}
//---------------------------------------------------------------------------
void __fastcall TMyThread::SyncListFileClear()
{
  Form1->ListFile->Items->Clear();
}
//---------------------------------------------------------------------------
void __fastcall TMyThread::SyncListFileAdd()
{
  TListItem *item = Form1->ListFile->Items->Add();
  item->Caption = File;
  item->SubItems->Add(ExtractFileExt(File));
  SHFILEINFO fi;
  SHGetFileInfo(File.c_str(), 0, &fi, sizeof(fi), SHGFI_SYSICONINDEX | SHGFI_USEFILEATTRIBUTES);
  item->ImageIndex = fi.iIcon;
}
//---------------------------------------------------------------------------
void __fastcall TMyThread::ListFileAdd(AnsiString file)
{
  File = file;
  Synchronize(SyncListFileAdd);
}
//---------------------------------------------------------------------------
void __fastcall TMyThread::SyncListFileAddDir()
{
  TListItem *item = Form1->ListFile->Items->Add();
  item->Caption = File;
  item->SubItems->Add("");
  SHFILEINFO fi;
  SHGetFileInfo("", FILE_ATTRIBUTE_DIRECTORY, &fi, sizeof(fi), SHGFI_SYSICONINDEX | SHGFI_USEFILEATTRIBUTES);
  item->ImageIndex = fi.iIcon;
}
//---------------------------------------------------------------------------
void __fastcall TMyThread::ListFileAddDir(AnsiString file)
{
  File = file;
  Synchronize(SyncListFileAddDir);
}
//---------------------------------------------------------------------------
void __fastcall TMyThread::SyncProgress1()
{
  Form1->ProgressBar1->Max = PMax1;
  Form1->ProgressBar1->Position = PPos1;
  Form1->LabelProgress->Caption = IntToStr(PPos1) + "/" + IntToStr(PMax1);
}
//---------------------------------------------------------------------------
void __fastcall TMyThread::Progress1(int pos, int max)
{
  if (max != -1)
    PMax1 = max;
  if (pos >= 0)
    PPos1 = pos;
  else
    PPos1 = PMax1 + pos;
  Synchronize(SyncProgress1);
}
//---------------------------------------------------------------------------
void __fastcall TMyThread::SyncProgress2()
{
  Form1->ProgressBar2->Max = PMax2;
  Form1->ProgressBar2->Position = PPos2;
  //Form1->LabelProgress->Caption = IntToStr(PPos2) + "/" + IntToStr(PMax2);
}
//---------------------------------------------------------------------------
void __fastcall TMyThread::Progress2(int pos, int max)
{
  if (max != -1)
    PMax2 = max;
  if (pos >= 0)
    PPos2 = pos;
  else
    PPos2 = PMax2 + pos;
  Synchronize(SyncProgress2);
}
//---------------------------------------------------------------------------
bool __fastcall TMyThread::SetState(HANDLE h, AnsiString state)
{
  DCB dcbCom;
  LogDebugAdd(state, NULL, 0);
  if (!GetCommState(h, &dcbCom))
  {
    DWORD err = GetLastError();
    LogDebugAdd("GetCommState Failed [("+IntToStr(err)+")"+SysErrorMessage(err)+"]", NULL, 0);
    return false;
  }
  if (!BuildCommDCB(state.c_str(), &dcbCom))
  {
    DWORD err = GetLastError();
    LogDebugAdd("BuildCommDCB Failed [("+IntToStr(err)+")"+SysErrorMessage(err)+"]", NULL, 0);
    return false;
  }
  //dcbCom.BaudRate = 600;
  //dcbCom.Parity = EVENPARITY;
  //dcbCom.ByteSize = 8;
  //dcbCom.StopBits = ONESTOPBIT;
  if (!SetCommState(h, &dcbCom))
  {
    DWORD err = GetLastError();
    LogDebugAdd("SetCommState Failed [("+IntToStr(err)+")"+SysErrorMessage(err)+"]", NULL, 0);
    return false;
  }
  if (!GetCommState(h, &dcbCom))
  {
    DWORD err = GetLastError();
    LogDebugAdd("GetCommState Failed [("+IntToStr(err)+")"+SysErrorMessage(err)+"]", NULL, 0);
    return false;
  }
  LogDebugAdd("baud=" + IntToStr(dcbCom.BaudRate) + " ok.", NULL, 0);
  return true;
}
//---------------------------------------------------------------------------
bool __fastcall TMyThread::WriteBuf(HANDLE h, unsigned char buf[], int len)
{
  unsigned long wrote;
  int pos = 0;
  while (!Terminated && pos < len)
  {
    Progress2(pos, len);
    WriteFile(h, buf + pos, len - pos, &wrote, NULL);
    if (wrote == 0)
      return false;
    FlushFileBuffers(h);
    LogDebugAdd("S", buf + pos, wrote);
    pos += wrote;
  }
  if (!Terminated && pos == len)
    return true;
  else
    return false;
}
//---------------------------------------------------------------------------
bool __fastcall TMyThread::ReadBuf(HANDLE h, unsigned char buf[], int len)
{
  int pos = 0;
  unsigned int t = GetTickCount();
  while (!Terminated && pos < len)
  {
    Progress2(pos, len);
    unsigned long read;
    if (!ReadFile(h, buf + pos, len - pos, &read, NULL))
      return false;
    if (read != 0)
    {
      LogDebugAdd("R", buf + pos, read);
      pos += read;
      t = GetTickCount();
    }
    else if (GetTickCount() - t > TimeOut)
      return false;
  }
  if (!Terminated && pos == len)
    return true;
  else
    return false;
}
//---------------------------------------------------------------------------
bool __fastcall TMyThread::Wait(HANDLE h, unsigned char data[], int len)
{
  unsigned char buf;
  int pos = 0;
  unsigned int t = GetTickCount();
  while (!Terminated && pos < len)
  {
    unsigned long read;
    if (!ReadFile(h, &buf, 1, &read, NULL))
      return false;
    if (read != 0)
    {
      LogDebugAdd("R", &buf, read);
      if (buf == data[pos])
        pos ++;
      else if (buf == data[0])
        pos = 1;
      else
      {
        pos = 0;
        continue;
      }
      t = GetTickCount();
    }
    else if (GetTickCount() - t > TimeOut)
      return false;
  }
  if (!Terminated && pos == len)
    return true;
  else
    return false;
}
//---------------------------------------------------------------------------
bool __fastcall TMyThread::Command(HANDLE h, unsigned short cmd,
        unsigned char data[], int len, unsigned char cont)
{
  unsigned char buf[1024] = {0x01, 0x00, 0x00, 0x00, 0x02, cmd>>8, cmd};

  // 引数コピー
  if (len > 0)
    CopyMemory(buf + 7, data, len);

  // データ長
  len += 6;
  buf[2] = len >> 8;
  buf[3] = len;

  len += 4 - 3;
  buf[len++] = cont;

  // チェックサム？
  unsigned short sum = 0;
  for (int i = 0; i < len; i ++)
    sum += buf[i];
  sum = ~sum;
  buf[len++] = sum>>8;
  buf[len++] = sum;

  int cnt;
  for (cnt = 0; cnt < 5 && !Terminated; cnt ++)
  {
    // コマンド送信
    if (!WriteBuf(h, buf, len))
      return false;

    unsigned char res;
    do
    {
      // 返事受信
      if (!ReadBuf(h, &res, 1))
        return false;
      if (res == 0x06)
        return true;
      if (res == 0x07)
        continue;
      if (res != 0x15)
        return false;
    } while (res == 0x07);

    Sleep(WaitCmd);
  }
  if (Terminated || cnt == 5)
    return false;

  return true;
}
//---------------------------------------------------------------------------
bool __fastcall TMyThread::Send(HANDLE h, unsigned char data[], int len, unsigned char cont)
{
  unsigned char buf1[5] = {0x01, 0x80, 0x00, 0x00, 0x02};
  unsigned char buf2[3] = {cont};

  // データ長
  len += 4;
  buf1[2] = len >> 8;
  buf1[3] = len;

  // チェックサム？
  unsigned short sum = cont;
  len -= 4;
  for (int i = 0; i < 5; i ++)
    sum += buf1[i];
  for (int i = 0; i < len; i ++)
    sum += data[i];
  sum = ~sum;
  buf2[1] = sum>>8;
  buf2[2] = sum;

  int cnt;
  for (cnt = 0; cnt < 5 && !Terminated; cnt ++)
  {
    // データ送信
    if (!WriteBuf(h, buf1, 5))
      return false;
    if (len > 0 && !WriteBuf(h, data, len))
      return false;
    if (!WriteBuf(h, buf2, 3))
      return false;

    unsigned char res;
    do
    {
      // 返事受信
      if (!ReadBuf(h, &res, 1))
        return false;
      if (res == 0x06)
        return true;
      if (res == 0x07)
        continue;
      if (res != 0x15)
        return false;
    } while (res == 0x07);

    Sleep(WaitCmd);
  }
  if (Terminated || cnt == 5)
    return false;

  return true;
}
//---------------------------------------------------------------------------
int __fastcall TMyThread::Receive(HANDLE h, unsigned char buf[])
{
  unsigned short sum = 0xFFFF;
  unsigned short dlen;
  do
  {
    if (Terminated)
      return -1;

    unsigned char sbuf[2];
    if (!Wait(h, "\x01\x80", 2))
      return -1;

    if (!ReadBuf(h, sbuf, 2))
    {
      WriteBuf(h, "\x15", 1);
      continue;
    }
    dlen = (sbuf[0]<<8) | sbuf[1];
    if (dlen > 0)
    {
      sum = 0x01 + 0x80 + sbuf[0] + sbuf[1];

      // 0x20 受信
      if (!ReadBuf(h, buf, 1))
      {
        WriteBuf(h, "\x15", 1);
        continue;
      }
      sum += buf[0];
      dlen --;

      if (!ReadBuf(h, buf, dlen))
      {
        WriteBuf(h, "\x15", 1);
        continue;
      }

      dlen -= 2;
      for (int i = 0; i < dlen; i ++)
        sum += buf[i];
      sum += (short)((buf[dlen]<<8) | buf[dlen+1]);
      if (sum != 0xFFFF)
        WriteBuf(h, "\x15", 1);
    }
  } while (sum != 0xFFFF && !Terminated);
  if (Terminated)
    return false;

  WriteBuf(h, "\x06", 1);
  return dlen;
}
//---------------------------------------------------------------------------
void __fastcall TMyThread::Execute()
{
  unsigned char buf[0x10000];
  unsigned long len, read;

  LogAdd("Thread Started.");

  // ポートオープン
  hCom = CreateFile(Port.c_str(), GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
  if (hCom == INVALID_HANDLE_VALUE)
  {
    LogAdd("ERROR: CAN'T OPEN '" + Port + "'");
    return;
  }
  LogDebugAdd(Port + " OPENED", NULL, 0);

  // バッファサイズ設定
  SetupComm(hCom, 4096, 4096);

  // 基本設定
  SetState(hCom, "baud=600 parity=e data=8 stop=1");

  // タイムアウト設定
  /*
  COMMTIMEOUTS comTimeout;
  comTimeout.ReadIntervalTimeout = 500;
  comTimeout.ReadTotalTimeoutConstant = 5000;
  comTimeout.ReadTotalTimeoutMultiplier = 10;
  comTimeout.WriteTotalTimeoutConstant = 5000;
  comTimeout.WriteTotalTimeoutMultiplier = 100;
  SetCommTimeouts(hCom, &comTimeout);
  //*/
  COMMTIMEOUTS comTimeout;
  comTimeout.ReadIntervalTimeout = RI;
  comTimeout.ReadTotalTimeoutConstant = RC;
  comTimeout.ReadTotalTimeoutMultiplier = RM;
  comTimeout.WriteTotalTimeoutConstant = WC;
  comTimeout.WriteTotalTimeoutMultiplier = WM;
  SetCommTimeouts(hCom, &comTimeout);

  // ATコマンド
  if (atcmd != "")
  {
    Sleep(WaitFirst);
    LogAdd("ATコマンド送信...");
    WriteBuf(hCom, (atcmd + "\r\n").c_str(), (atcmd + "\r\n").Length());
    if (!Wait(hCom, "\r\n", 2))
    {
      LogAdd("ATコマンド失敗");
      LogAdd("モデムコードではないか");
      LogAdd("コマンドが違う可能性があります");
      return;
    }
    //Wait(hCom, "\r\n", 2);

    char temp[32];
    unsigned long tread;
    unsigned int tt = GetTickCount();
    while (GetTickCount() - tt < WaitAT)
    {
      ReadFile(hCom, temp, 32, &tread, NULL);
      if (tread != 0)
        LogDebugAdd("R", temp, tread);
    }

    //LogAdd("ATコマンド成功");
  }

  unsigned int t = GetTickCount() - 3000 + WaitFirst;
  do
  {
    Synchronize(SyncGetTask);
    if (Task == "GetTelephoneNumber")
    {
      // 電話番号取得
      LogAdd("GetTelephoneNumber...");
      AnsiString telno = "";
      while (1)
      {
        if (telno == "" && GetTickCount() - t > 1000)
        {
          t = GetTickCount();
          WriteBuf(hCom, "\x8E", 1);
        }
        if (Terminated || !ReadFile(hCom, buf, 1, &read, NULL))
        {
          LogAdd("取得失敗");
          return;
        }
        if (read > 0)
        {
          LogDebugAdd("R", buf, read);
          if (0x91 <= buf[0] && buf[0] <= 0x9A)
          {
            if (buf[0] != 0x9A)
              buf[0] = buf[0] - 0x90 + 0x30;
            else
              buf[0] = '0';
            buf[1] = 0;
            telno += AnsiString((char*)buf);
          }
          else if (buf[0] == 0xAA)
            break;
        }
      }
      LogAdd("TELNO: " + telno);
      Synchronize(SyncDelTask);
    }
    else if (Task == "SearchLockNumber")
    {
      LogAdd("SearchLockNumber...");
      unsigned char sno[8] = "\xB3\x01\x9A\x9A\x9A\x9A\x86";

      int i = 0;
      Progress1(i, 1000);
      for (sno[2] = 0x91; sno[2] <= 0x9A && !Terminated; sno[2] ++)
        for (sno[3] = 0x91; sno[3] <= 0x9A && !Terminated; sno[3] ++)
          for (sno[4] = 0x91; sno[4] <= 0x9A && !Terminated; sno[4] ++)
          {
            for (sno[5] = 0x91; sno[5] <= 0x9A && !Terminated; sno[5] ++)
              WriteBuf(hCom, sno, 7);
            for (sno[5] = 0x91; sno[5] <= 0x9A && !Terminated; sno[5] ++)
            {
              do
              {
                if (!ReadBuf(hCom, buf, 1))
                {
                  LogAdd("検索失敗");
                  return;
                }
                if (buf[0] == 0x86)
                {
                  ReadBuf(hCom, buf, 1);
                  //goto found;
                  AnsiString n = "";
                  for (int i = 2; i < 6; i ++)
                    if (sno[i] != 0x9A)
                      n += AnsiString((char)(sno[i] - 0x90 + 0x30));
                    else
                      n += "0";
                  LogAdd("LOCKNO: " + n);
                  break;
                }
              } while (buf[0] != 0x87 && !Terminated);
            }
            //LogAdd("Searched... " + IntToStr(++i) + "0 nums");
            Progress1(++i);
          }
      //found:;
      if (Terminated)
      {
        LogAdd("検索中止");
        return;
      }

      //for (int i = 2; i < 6; i ++)
      //  if (sno[i] != 0x9A)
      //    sno[i] = sno[i] - 0x90 + 0x30;
      //  else
      //    sno[i] = '0';
      //sno[6] = 0;
      //LogAdd("LOCKNO: " + AnsiString((char*)sno+2));
      Synchronize(SyncDelTask);
    }
  } while (Task == "GetTelephoneNumber" || Task == "SearchLockNumber" && !Terminated);

  //Synchronize(SyncGetTask);
  if (Terminated || Task == "")
    return;

  // 転送モード
  LogAdd("モード移行...");
  len = 0;
  while (len < 3)
  {
    if (len == 0 && GetTickCount() - t > 3000)
    {
      t = GetTickCount();
      WriteBuf(hCom, "\xD0\x01\x05", 3);
    }

    if (Terminated || !ReadFile(hCom, buf, 1, &read, NULL))
    {
      LogAdd("移行失敗(1)");
      return;
    }
    if (read > 0)
    {
      LogDebugAdd("R", buf, read);
      if (len == 2)
      {
        if (buf[0] != 0x06)
        {
          LogAdd("移行失敗(1) " + IntToHex(buf[0], 2));
          return;
        }
        len ++;
      }
      else
      {
        if (buf[0] == (unsigned char)("\xD0\x01")[len])
          len ++;
        else if (buf[0] == 0xD0)
          len = 1;
        else
          len = 0;
      }
    }
  }

  Sleep(1000);
  WriteBuf(hCom, "\x05", 1);
  if (!ReadBuf(hCom, buf, 1) || buf[0] != 0x06)
  {
    LogAdd("移行失敗(2) " + IntToHex(buf[0], 2));
    return;
  }

  // SetProtocolVersion{Version(4B)}
  if (!Command(hCom, 0x0000, "\x00\x01\x00\x00", 4))
  {
    WriteBuf(hCom, "\x04", 1);
    LogAdd("SetProtocolVersion失敗");
    return;
  }

  // SetBaudRate{BaudRate(4B)}
  do
  {
    // 使えるボードレートの探索
    int rate = BaudRate;
    while (!SetState(hCom, "baud=" + IntToStr(rate) + " parity=e data=8 stop=1"))
    {
      if (rate >= 115200) rate /= 2;
      else if (rate >= 57600) rate = 38400; //BaudRate * 2 / 3;
      else if (rate >= 38400) rate = 28800; //BaudRate * 3 / 4;
      else if (rate >= 28800) rate = 19200; //BaudRate * 2 / 3;
      else if (rate >= 19200) rate = 14400; //BaudRate * 3 / 4;
      else if (rate >= 14400) rate = 9600; //BaudRate * 2 / 3;
      else if (rate >= 4800) rate /= 2;
      else
      {
        rate /= 4;
        break;
      }
    }
    // ボーレート元に戻す
    SetState(hCom, "baud=600 parity=e data=8 stop=1");

    if (rate < 600)
    {
      WriteBuf(hCom, "\x04", 1);
      LogAdd("SetBaudRate失敗");
      return;
    }
    BaudRate = rate;
    buf[0] = BaudRate >> 24;
    buf[1] = BaudRate >> 16;
    buf[2] = BaudRate >> 8;
    buf[3] = BaudRate;
  } while (!Command(hCom, 0x0001, buf, 4));
  Sleep(WaitBaud / 5);
  SetState(hCom, "baud=" + IntToStr(BaudRate) + " parity=e data=8 stop=1");
  Sleep(WaitBaud * 4 / 5);

  // SetFrameSize{FrameSize(4B)}
  buf[0] = FrameSize >> 24;
  buf[1] = FrameSize >> 16;
  buf[2] = FrameSize >> 8;
  buf[3] = FrameSize;
  while (!Command(hCom, 0x0003, buf, 4))
  {
    FrameSize /= 2;
    if (FrameSize < 256)
    {
      WriteBuf(hCom, "\x04", 1);
      LogAdd("SetFrameSize失敗");
      return;
    }
    buf[0] = FrameSize >> 24;
    buf[1] = FrameSize >> 16;
    buf[2] = FrameSize >> 8;
    buf[3] = FrameSize;
  }
  
  do
  {
    if (Task == "GetProfile(0000h)")
    {
      // GetProfile{Number(2B):0000h}
      LogAdd("GetProfile(0000h)...");
      if (!Command(hCom, 0x0204, "\x00\x00", 2) || Receive(hCom, buf) == -1)
      {
        WriteBuf(hCom, "\x04", 1);
        LogAdd("GetProfile(0000h)失敗");
        return;
      }

      char name1[256], name2[256];
      int pos = 2 + ((buf[0]<<8) | buf[1]) * 2;
      unsigned short len1 = (buf[pos]<<8) | buf[pos+1];
      pos += 2;
      CopyMemory(name1, buf + pos, len1);
      name1[len1] = 0;
      pos += len1;
      unsigned short len2 = (buf[pos]<<8) | buf[pos+1];
      pos += 2;
      CopyMemory(name2, buf + pos, len2);
      name2[len2] = 0;
      pos += len2;

      LogAdd("タイプ数: " + IntToStr((buf[0]<<8) | buf[1]));
      //LogAdd("タイプ: " + IntToStr((buf[2]<<8) | buf[3]) + ", " + IntToStr((buf[4]<<8) | buf[5]));
      LogAdd(AnsiString(name1) + " " + AnsiString(name2) + " ver." + IntToHex((buf[pos]<<24) | (buf[pos+1]<<16) | (buf[pos+2]<<8) | buf[pos+3], 8));
      pos += 4;
      pos += 4; // 予約
      LogAdd("受信可能サイズ: " + IntToStr((buf[pos]<<24) | (buf[pos+1]<<16) | (buf[pos+2]<<8) | buf[pos+3]));
      pos += 4;
      LogAdd("コマンド数: " + IntToStr((buf[pos]<<8) | buf[pos+1]));
    }
    else if (Task == "GetProfile(0100h)")
    {
      // GetProfile{Number(2B):0100h}
      LogAdd("GetProfile(0100h)...");
      if (!Command(hCom, 0x0204, "\x01\x00", 2) || Receive(hCom, buf) == -1)
      {
        WriteBuf(hCom, "\x04", 1);
        LogAdd("GetProfile(0100h)失敗");
        return;
      }

      int pos = 0;
      LogAdd("アドレスサポート: " + IntToStr(buf[pos++]));
      LogAdd("グループサポート: " + IntToStr(buf[pos++]));
      LogAdd("スケジュールサポート: " + IntToStr(buf[pos++]));
      LogAdd("タスクサポート: " + IntToStr(buf[pos++]));
      LogAdd("E-mailサポート: " + IntToStr(buf[pos++]));
      LogAdd("C-mailサポート: " + IntToStr(buf[pos++]));
      LogAdd("SkyMessageサポート: " + IntToStr(buf[pos++]));

      LogAdd("アドレスメモリサイズ: " + IntToStr((buf[pos]<<24) | (buf[pos+1]<<16) | (buf[pos+2]<<8) | buf[pos+3]));
      pos += 4;
      LogAdd("スケジュールメモリサイズ: " + IntToStr((buf[pos]<<24) | (buf[pos+1]<<16) | (buf[pos+2]<<8) | buf[pos+3]));
      pos += 4;
      LogAdd("タスクメモリサイズ: " + IntToStr((buf[pos]<<24) | (buf[pos+1]<<16) | (buf[pos+2]<<8) | buf[pos+3]));
      pos += 4;
      LogAdd("E-mailメモリサイズ: " + IntToStr((buf[pos]<<24) | (buf[pos+1]<<16) | (buf[pos+2]<<8) | buf[pos+3]));
      pos += 4;
      LogAdd("C-mailメモリサイズ: " + IntToStr((buf[pos]<<24) | (buf[pos+1]<<16) | (buf[pos+2]<<8) | buf[pos+3]));
      pos += 4;
      pos += 4; // 予約
      LogAdd("SkyMessage最大数: " + IntToStr((buf[pos]<<24) | (buf[pos+1]<<16) | (buf[pos+2]<<8) | buf[pos+3]));
      pos += 4;
      LogAdd("FunctionTransferサポート: " + IntToStr(buf[pos++]));
      LogAdd("コマンド数: " + IntToStr((buf[pos]<<8) | buf[pos+1]));
      pos += 2 + ((buf[pos]<<8) | buf[pos+1]) * 2;
      LogAdd("コントロール数: " + IntToStr((buf[pos]<<8) | buf[pos+1]));
    }
    else if (Task == "GetFileList")
    {
      // GetFileList{}
      LogAdd("GetFileList...");
      Synchronize(SyncListFileClear);
      if (!Command(hCom, 0x0214, NULL, 0))
      {
        WriteBuf(hCom, "\x04", 1);
        LogAdd("GetFileList失敗");
        return;
      }
      int n = -1;
      char fname[256];
      int p2 = 0;
      do
      {
        int dlen;
        if ((dlen = Receive(hCom, buf)) == -1)
        {
          WriteBuf(hCom, "\x04", 1);
          LogAdd("GetFileListデータ取得失敗");
          return;
        }

        int p = 0;
        if (n == -1)
        {
          n = (buf[0]<<8) | buf[1];
          p += 2;
          Progress1(-n, n);
        }

        dlen --;
        while (!Terminated && p < dlen)
        {
          do
            fname[p2++] = buf[p];
          while (!Terminated && buf[p++] && p < dlen);
          if (fname[p2-1] == 0)
          {
            ListFileAdd(fname);
            p2 = 0;
            n --;
          }
        }
        Progress1(-n);
      } while (!Terminated && n > 0);
      if (Terminated)
      {
        WriteBuf(hCom, "\x04", 1);
        LogAdd("GetFileList失敗");
        return;
      }
    }
    else if (Task.SubString(1, 7) == "GetFile")
    {
      unsigned short gfcmd = 0x0210;
      int filesize;

      AnsiString age;
      if (ExtractFileDir(RemoteFile) == "")
      {
        // SetFile{FileName(STR\0)}
        LogAdd("SetFile(" + RemoteFile + ")");
        if (!Command(hCom, 0x0010, RemoteFile.c_str(), RemoteFile.Length()+1))
        {
          WriteBuf(hCom, "\x04", 1);
          LogAdd("SetFile失敗");
          return;
        }

        // GetFileInfo{}
        LogAdd("GetFileInfo...");
        if (!Command(hCom, 0x0212, "\x00\x00", 2) || Receive(hCom, buf) == -1)
        {
          WriteBuf(hCom, "\x04", 1);
          LogAdd("GetFileInfo失敗");
          return;
        }

        AnsiString fn = (char*)buf;
        int pos = fn.Length() + 5;
        filesize = (buf[pos]<<24) | (buf[pos+1]<<16) | (buf[pos+2]<<8) | buf[pos+3];
        buf[pos + 4 + 14] = 0;
        age = (char*)(buf + pos + 4);
        LogAdd("INFO: FileName=" + fn + " FileSize=" + IntToStr(filesize) + " Date=" + age);
      }
      else
      {
        /*
        AnsiString d = ExtractFileDir(RemoteFile);
        RemoteFile = ExtractFileName(RemoteFile);
        if (d[1] == '\\' && d.Length() != 1)
          d = d.SubString(2, d.Length() - 1);

        // SDSetDir{Path(STR\0)}
        LogAdd("SetDirSD(" + d + ")...");
        len = d.Length()+1;
        CopyMemory(buf, d.c_str(), len);
        if (!Command(hCom, 0x00B1, buf, len))
        {
          WriteBuf(hCom, "\x04", 1);
          LogAdd("SDSetDir失敗");
          return;
        }
        */
        gfcmd = 0x02B0;
        filesize = 0;
      }

      // GetFile{FileName(STR\0)}
      LogAdd("GetFile(" + RemoteFile + ")...");
      if (!Command(hCom, gfcmd, RemoteFile.c_str(), RemoteFile.Length()+1))
      {
        WriteBuf(hCom, "\x04", 1);
        LogAdd("GetFile失敗");
        return;
      }
      
      TMemoryStream *mem = new TMemoryStream;
      try
      {
        int dlen;
        Progress1(mem->Position, filesize);
        do
        {
          if ((dlen = Receive(hCom, buf)) == -1)
          {
            delete mem;
            WriteBuf(hCom, "\x04", 1);
            LogAdd("GetFile失敗");
            return;
          }
          dlen --;
          mem->Write(buf, dlen);
          //LogAdd("Receiving... " + IntToStr(mem->Position) + "/" + IntToStr(filesize));
          Progress1(mem->Position, filesize);
        } while (buf[dlen] == 0x17 && !Terminated);
        if (Terminated)
        {
          delete mem;
          WriteBuf(hCom, "\x04", 1);
          LogAdd("GetFile失敗");
          return;
        }

        AnsiString lfile = LocalFile;
        int n = 1;
        while (FileExists(lfile) && n < 100)
          lfile = ChangeFileExt(lfile,"") + "(" + IntToStr(n++) + ")" + ExtractFileExt(lfile);
        mem->SaveToFile(lfile);

        // タイムスタンプ設定
        HANDLE h = CreateFile(LocalFile.c_str(), GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
        if (h)
        {
          try {
            age = age.SubString(1, 4) + "/" + age.SubString(5, 2) + "/" + age.SubString(7, 2)
                + " " + age.SubString(9, 2) + ":" + age.SubString(11, 2) + ":" + age.SubString(13, 2);
            FILETIME lft, ft;
            int a = DateTimeToFileDate(age);
            DosDateTimeToFileTime(a >> 16, a, &lft);
            LocalFileTimeToFileTime(&lft, &ft);
            SetFileTime(h, &ft, &ft, &ft);
          } catch (...) { }
          CloseHandle(h);
        }
      }
      catch (...)
      {
        LogAdd("SaveToFile失敗");
      }
      delete mem;

      if (ExtractFileDir(RemoteFile) != "")
      {
        // SDSetDir{Path(STR\0)}
        LogAdd("SetDirSD(\\)...");
        if (!Command(hCom, 0x00B1, "\\", 2))
        {
          WriteBuf(hCom, "\x04", 1);
          LogAdd("SDSetDir失敗");
          return;
        }
      }
    }
    else if (Task.SubString(1, 7) == "PutFile")
    {
      unsigned short pfcmd = 0x0110;
      
      if (ExtractFileDir(RemoteFile) != "")
      {
        // SDSetDir{Path(STR\0)}
        LogAdd("SetDirSD(\\)...");
        if (!Command(hCom, 0x00B1, "\\", 2))
        {
          WriteBuf(hCom, "\x04", 1);
          LogAdd("SDSetDir失敗");
          return;
        }

        AnsiString d = ExtractFileDir(RemoteFile);
        RemoteFile = ExtractFileName(RemoteFile);
        if (d[1] == '\\' && d.Length() != 1)
          d = d.SubString(2, d.Length() - 1);

        // SDSetDir{Path(STR\0)}
        LogAdd("SetDirSD(" + d + ")...");
        len = d.Length()+1;
        CopyMemory(buf, d.c_str(), len);
        if (!Command(hCom, 0x00B1, buf, len))
        {
          WriteBuf(hCom, "\x04", 1);
          LogAdd("SDSetDir失敗");
          return;
        }
        pfcmd = 0x01B0;
      }

      // PutFile{FileName(STR\0),FileSize(4B)}
      LogAdd("PutFile(" + RemoteFile + ")...");
      TMemoryStream *mem = new TMemoryStream;
      try
      {
        mem->LoadFromFile(LocalFile);

        len = RemoteFile.Length()+1;
        CopyMemory(buf, RemoteFile.c_str(), len);
        buf[len++] = mem->Size >> 24;
        buf[len++] = mem->Size >> 16;
        buf[len++] = mem->Size >> 8;
        buf[len++] = mem->Size;

        if (!Command(hCom, pfcmd, buf, len))
        {
          delete mem;
          WriteBuf(hCom, "\x04", 1);
          LogAdd("PutFile失敗");
          return;
        }

        Progress1(mem->Position, mem->Size);
        do
        {
          len = mem->Read(buf, FrameSize - 10);
          if (!Send(hCom, buf, len, mem->Position < mem->Size ? 0x17 : 0x03))
          {
            delete mem;
            WriteBuf(hCom, "\x04", 1);
            LogAdd("PutFile失敗");
            return;
          }
          //LogAdd("Sending... " + IntToStr(mem->Position) + "/" + IntToStr(mem->Size));
          Progress1(mem->Position, mem->Size);
        } while (mem->Position < mem->Size && !Terminated);
        if (Terminated)
        {
          delete mem;
          WriteBuf(hCom, "\x04", 1);
          LogAdd("PutFile失敗");
          return;
        }
      }
      catch (...)
      {
        LogAdd("LoadFromFile失敗");
      }
      delete mem;

      if (ExtractFileDir(RemoteFile) != "")
      {
        // SDSetDir{Path(STR\0)}
        LogAdd("SetDirSD(\\)...");
        if (!Command(hCom, 0x00B1, "\\", 2))
        {
          WriteBuf(hCom, "\x04", 1);
          LogAdd("SDSetDir失敗");
          return;
        }
      }
    }
    else if (Task.SubString(1, 7) == "DelFile")
    {
      unsigned short pfcmd = 0x0110;
      
      if (ExtractFileDir(RemoteFile) != "")
      {
        // SDSetDir{Path(STR\0)}
        LogAdd("SetDirSD(\\)...");
        if (!Command(hCom, 0x00B1, "\\", 2))
        {
          WriteBuf(hCom, "\x04", 1);
          LogAdd("SDSetDir失敗");
          return;
        }

        AnsiString d = ExtractFileDir(RemoteFile);
        RemoteFile = ExtractFileName(RemoteFile);
        if (d[1] == '\\' && d.Length() != 1)
          d = d.SubString(2, d.Length() - 1);

        // SDSetDir{Path(STR\0)}
        LogAdd("SetDirSD(" + d + ")...");
        len = d.Length()+1;
        CopyMemory(buf, d.c_str(), len);
        if (!Command(hCom, 0x00B1, buf, len))
        {
          WriteBuf(hCom, "\x04", 1);
          LogAdd("SDSetDir失敗");
          return;
        }
        pfcmd = 0x01B0;
      }

      // PutFile{FileName(STR\0),FileSize(4B)}
      LogAdd("DelFile(" + RemoteFile + ")...");
      len = RemoteFile.Length()+1;
      CopyMemory(buf, RemoteFile.c_str(), len);
      ZeroMemory(buf + len, 4);
      len += 4;
      if (!Command(hCom, pfcmd, buf, len))
      {
        WriteBuf(hCom, "\x04", 1);
        LogAdd("DelFile失敗");
        return;
      }

      if (!Send(hCom, NULL, 0, 0x03))
      {
        WriteBuf(hCom, "\x04", 1);
        LogAdd("DelFile失敗");
        return;
      }

      if (ExtractFileDir(RemoteFile) != "")
      {
        // SDSetDir{Path(STR\0)}
        LogAdd("SetDirSD(\\)...");
        if (!Command(hCom, 0x00B1, "\\", 2))
        {
          WriteBuf(hCom, "\x04", 1);
          LogAdd("SDSetDir失敗");
          return;
        }
      }
    }
    else if (Task.SubString(1, 13) == "SDGetFileList")
    {
      Synchronize(SyncListFileClear);

      /*
      AnsiString path = RemoteFile;
      do
      {
        AnsiString pn;
        int p = path.Pos("\\");
        if (p == 0) p = path.Length()+1;
        if (p == 1)
          pn = "\\";
        else
          pn = path.SubString(1, p-1);
        path = path.SubString(p+1, path.Length());

        // SDSetDir{Path(STR\0)}
        LogAdd("SDSetDir(" + pn + ")...");
        len = pn.Length()+1;
        CopyMemory(buf, pn.c_str(), len);
        if (!Command(hCom, 0x00B1, buf, len))
        {
          WriteBuf(hCom, "\x04", 1);
          LogAdd("SDSetDir失敗");
          return;
        }
      } while (path != "");
      */
      // SDSetDir{Path(STR\0)}
      LogAdd("SDSetDir(" + RemoteFile + ")...");
      len = RemoteFile.Length()+1;
      CopyMemory(buf, RemoteFile.c_str(), len);
      if (!Command(hCom, 0x00B1, buf, len))
      {
        WriteBuf(hCom, "\x04", 1);
        LogAdd("SDSetDir失敗");
        return;
      }

      if (RemoteFile[RemoteFile.Length()] != '\\')
        RemoteFile += "\\";

      // SDGetDirInfo?
      //if (!Command(hCom, 0x02B6, NULL, 0))
      //{
      //  WriteBuf(hCom, "\x04", 1);
      //  LogAdd("SDGetDirInfo失敗");
      //  return;
      //}
      //if ((dlen = Receive(hCom, buf)) == -1)
      //{
      //  WriteBuf(hCom, "\x04", 1);
      //  LogAdd("SDGetDirInfoデータ取得失敗");
      //  return;
      //}

      // SDGetFileList
      LogAdd("SDGetFileList...");
      if (!Command(hCom, 0x02B4, NULL, 0))
      {
        WriteBuf(hCom, "\x04", 1);
        LogAdd("SDGetFileList失敗");
        return;
      }
      int n = -1;
      do
      {
        int dlen;
        if ((dlen = Receive(hCom, buf)) == -1)
        {
          WriteBuf(hCom, "\x04", 1);
          LogAdd("SDGetFileListデータ取得失敗");
          return;
        }

        int p = 0;
        if (n == -1)
        {
          n = (buf[0]<<8) | buf[1];
          p += 2;
          Progress1(-n, n);
        }

        dlen --;
        while (!Terminated && p < dlen)
        {
          unsigned short kind = (buf[p]<<8) | buf[p+1];
          p += 2;
          //unsigned int size = (buf[p]<<24) | (buf[p+1]<<16) | (buf[p+2]<<8) | buf[p+3];
          p += 4;
          //AnsiString date = AnsiString((char*)buf+p).SubString(1,16);
          p += 16;
          if (kind == 0x0000) // ファイル
            ListFileAdd(RemoteFile + AnsiString((char*)buf + p));
          else if (kind == 0x0002) // ディレクトリ
            ListFileAddDir(RemoteFile + AnsiString((char*)buf + p) + "\\");
          p += strlen(buf + p) + 1;
          //
          p += strlen(buf + p) + 1;
          n --;
        }
        Progress1(-n);
      } while (!Terminated && n > 0);
      if (Terminated)
      {
        WriteBuf(hCom, "\x04", 1);
        LogAdd("SDGetFileList失敗");
        return;
      }

      // SDSetDir{Path(STR\0)}
      LogAdd("SDSetDir(\\)...");
      if (!Command(hCom, 0x00B1, "\\", 2))
      {
        WriteBuf(hCom, "\x04", 1);
        LogAdd("SDSetDir失敗");
        return;
      }
    }
    else
    {
      WriteBuf(hCom, "\x04", 1);
      LogAdd("Unknown Task: " + Task);
      return;
    }

    Synchronize(SyncDelTask);
    Synchronize(SyncGetTask);
    Sleep(WaitTask);
  } while (Task != "" && !Terminated);

  // 終了
  WriteBuf(hCom, "\x04", 1);

  CloseHandle(hCom);
  hCom = INVALID_HANDLE_VALUE;
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
__fastcall TForm1::TForm1(TComponent* Owner)
        : TForm(Owner)
{
}
//---------------------------------------------------------------------------
void __fastcall TForm1::ButtonConnectClick(TObject *Sender)
{
  if (MyThread)
  {
    MyThread->Terminate();
    //delete MyThread;
    //MyThread = NULL;

    //ButtonConnect->Caption = "&Connect";
    //Log->Lines->Add("Thread Terminated.");
    return;
  }

  if (ListTask->Items->Count == 0)
  {
    //Application->MessageBox("Add tasks.", "connect", MB_ICONINFORMATION | MB_OK);
    //return;
    ListTask->Items->AddObject("GetTelephoneNumber", NULL);
    ListTask->Items->AddObject("GetFileList", NULL);
  }

  AnsiString at = "";
  if (ComboCable->ItemIndex == -1)
    at = ComboCable->Text;
  else if (ComboCable->ItemIndex == 1)
    at = "AT%Z";
  else if (ComboCable->ItemIndex == 2)
    at = "ATZ";
  else if (ComboCable->ItemIndex == 3)
    at = "ATZ80";
  MyThread = new TMyThread(true, ComboPort->Text, at, StrToIntDef(EditBaud->Text, 115200), StrToIntDef(EditFrame->Text, 0x400));
  MyThread->Debug = Debug;
  if (tout != -1) MyThread->TimeOut = tout;
  if (w1 != -1) MyThread->WaitFirst = w1;
  if (wat != -1) MyThread->WaitAT = wat;
  if (wbaud != -1) MyThread->WaitBaud = wbaud;
  if (wcmd != -1) MyThread->WaitCmd = wcmd;
  if (wtask != -1) MyThread->WaitTask = wtask;
  if (RI != -1) MyThread->RI = RI;
  if (RM != -1) MyThread->RM = RM;
  if (RC != -1) MyThread->RC = RC;
  if (WM != -1) MyThread->WM = WM;
  if (WC != -1) MyThread->WC = WC;
  MyThread->OnTerminate = ThreadTerminate;
  MyThread->FreeOnTerminate = true;
  MyThread->Resume();
  ButtonConnect->Caption = "Dis&connect";
  //Log->Lines->Add("Thread Started.");
}
//---------------------------------------------------------------------------
void __fastcall TForm1::ThreadTerminate(TObject *Sender)
{
  MyThread = NULL;

  ProgressBar1->Position = 0;
  LabelProgress->Caption = "0/0";
  ProgressBar2->Position = 0;
  ButtonConnect->Caption = "&Connect";
  Log->Lines->Add("Thread Terminated.");
  Log->Lines->Add("");
}
//---------------------------------------------------------------------------
void __fastcall TForm1::FormCreate(TObject *Sender)
{
  IniFile = ChangeFileExt(Application->ExeName, ".ini");
  Debug = false;
  w1 = wbaud = wcmd = wtask = tout = -1;
  MyThread = NULL;
  ButtonClearClick(Sender);
  DragAcceptFiles(Handle, true);
  ComboCmd->ItemIndex = ComboCmd->Items->IndexOf("GetFileList");

  try
  {
    TIniFile *Reg = new TIniFile(IniFile);
    try
    {
      Width = Reg->ReadInteger("KTrans", "Width", Width);
      Height = Reg->ReadInteger("KTrans", "Height", Height);
      ListFile->Width = Reg->ReadInteger("KTrans", "FileWidth", ListFile->Width);
      Log->Height = Reg->ReadInteger("KTrans", "LogHeight", Log->Height);

      ComboPort->Text = Reg->ReadString("KTrans", "Port", ComboPort->Text);
      ComboCable->ItemIndex = Reg->ReadInteger("KTrans", "Cable", ComboCable->ItemIndex);
      if (ComboCable->ItemIndex == -1)
        ComboCable->Text = Reg->ReadString("KTrans", "CableText", ComboCable->Text);
      EditBaud->Text = Reg->ReadString("KTrans", "BaudRate", EditBaud->Text);
      EditFrame->Text = Reg->ReadString("KTrans", "FrameSize", EditFrame->Text);

      OpenDialog->InitialDir = Reg->ReadString("KTrans", "OpenDir", OpenDialog->InitialDir);
      SaveDialog->InitialDir = Reg->ReadString("KTrans", "SaveDir", SaveDialog->InitialDir);

      Reg->ReadSections(ComboMobile->Items);
      Debug = Reg->ReadBool("KTrans", "Debug", false);
      w1 = Reg->ReadInteger("KTrans", "WaitFirst", -1);
      wat = Reg->ReadInteger("KTrans", "WaitAT", -1);
      wbaud = Reg->ReadInteger("KTrans", "WaitBaud", -1);
      wcmd = Reg->ReadInteger("KTrans", "WaitCmd", -1);
      wtask = Reg->ReadInteger("KTrans", "WaitTask", -1);
      tout = Reg->ReadInteger("KTrans", "TimeOut", -1);

      RI = Reg->ReadInteger("KTrans", "RI", -1);
      RM = Reg->ReadInteger("KTrans", "RM", -1);
      RC = Reg->ReadInteger("KTrans", "RC", -1);
      WM = Reg->ReadInteger("KTrans", "WM", -1);
      WC = Reg->ReadInteger("KTrans", "WC", -1);

      for (int i = 0; i < ListFile->Columns->Count; i ++)
        ListFile->Columns->Items[i]->Width = Reg->ReadInteger("KTrans", "Column" + IntToStr(i) + "Width", ListFile->Columns->Items[i]->Width);
    }
    __finally
    {
      delete Reg;
    }
  }
  catch (...)
  {
  }

  SHFILEINFO fi;
  //IconListLarge->ShareImages = true;
  //IconListLarge->Handle = SHGetFileInfo(NULL, 0, &fi, sizeof(fi), SHGFI_SYSICONINDEX);
  IconListSmall->ShareImages = true;
  IconListSmall->Handle = SHGetFileInfo("", 0, &fi, sizeof(fi), SHGFI_SYSICONINDEX | SHGFI_SMALLICON);
}
//---------------------------------------------------------------------------
void __fastcall TForm1::FormCloseQuery(TObject *Sender, bool &CanClose)
{
  try
  {
    TIniFile *Reg = new TIniFile(IniFile);
    try
    {
      Reg->WriteInteger("KTrans", "Width", Width);
      Reg->WriteInteger("KTrans", "Height", Height);
      Reg->WriteInteger("KTrans", "FileWidth", ListFile->Width);
      Reg->WriteInteger("KTrans", "LogHeight", Log->Height);

      Reg->WriteString("KTrans", "Port", ComboPort->Text);
      Reg->WriteInteger("KTrans", "Cable", ComboCable->ItemIndex);
      if (ComboCable->ItemIndex == -1)
        Reg->WriteString("KTrans", "CableText", ComboCable->Text);
      Reg->WriteString("KTrans", "BaudRate", EditBaud->Text);
      Reg->WriteString("KTrans", "FrameSize", EditFrame->Text);

      Reg->WriteString("KTrans", "OpenDir", OpenDialog->InitialDir);
      Reg->WriteString("KTrans", "SaveDir", SaveDialog->InitialDir);

      for (int i = 0; i < ListFile->Columns->Count; i ++)
        Reg->WriteInteger("KTrans", "Column" + IntToStr(i) + "Width", ListFile->Columns->Items[i]->Width);
    }
    __finally
    {
      delete Reg;
    }
  }
  catch (...)
  {
  }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::FormDestroy(TObject *Sender)
{
  if (MyThread)
    MyThread->Terminate();
  while (ListTask->Items->Count > 0)
  {
    if (ListTask->Items->Objects[0])
      delete (TTaskInfo*)ListTask->Items->Objects[0];
    ListTask->Items->Delete(0);
  }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::WMDropFiles(TWMDropFiles &Msg)
{
  HDROP hDrop = (HDROP)Msg.Drop;
  if (MyThread)
  {
    DragFinish(hDrop);
    return;
  }

  int n = DragQueryFile(hDrop, 0xFFFFFFFF, NULL, 0);
  for (int i = 0; i < n; i ++)
  {
    char buf[MAX_PATH];
    DragQueryFile(hDrop, i, buf, MAX_PATH);

    TTaskInfo *task = new TTaskInfo();
    task->RemoteFile = ExtractFileName(buf);
    task->LocalFile = buf;
    ListTask->Items->AddObject("PutFile(" + task->RemoteFile + ")", (TObject*)task);
  }
  DragFinish(hDrop);
}
//---------------------------------------------------------------------------
void __fastcall TForm1::LogChange(TObject *Sender)
{
  Log->Perform(EM_SCROLLCARET, 0, 0);
}
//---------------------------------------------------------------------------
void __fastcall TForm1::PanelTopResize(TObject *Sender)
{
  ButtonClear->Left = PanelTop->Width - 50;
}
//---------------------------------------------------------------------------
void __fastcall TForm1::PanelBottomResize(TObject *Sender)
{
  ProgressBar1->Width = (PanelBottom->Width - 120) / 4 * 3;
  ProgressBar2->Left = ProgressBar1->Left + ProgressBar1->Width + 5;
  ProgressBar2->Width = (PanelBottom->Width - 120) / 4;
  ButtonAdd->Left = PanelBottom->Width - 50;
  ComboCmd->Width = PanelBottom->Width - 160;
}
//---------------------------------------------------------------------------
void __fastcall TForm1::ButtonClearClick(TObject *Sender)
{
  Log->Lines->Clear();
  Log->Lines->Add("Yasu software - KTrans");
  Log->Lines->Add("");
}
//---------------------------------------------------------------------------
void __fastcall TForm1::ButtonAddClick(TObject *Sender)
{
  if (ComboCmd->ItemIndex == -1 || MyThread)
    return;

  AnsiString cmd = ComboCmd->Text;
  if (cmd == "GetFile")
  {
    KeyGetFileClick(Sender);
  }
  else if (cmd == "GetFile(InputFileName)")
  {
    AnsiString fname;
    if (InputQuery(cmd, "Input filename", fname) && fname != "")
    {
      if (fname[1] == '\\' && fname.Length() != 1)
        fname = fname.SubString(2, fname.Length() - 1);
      SaveDialog->FileName = ExtractFileName(fname);
      if (SaveDialog->Execute())
      {
        SaveDialog->InitialDir = ExtractFileDir(SaveDialog->FileName);
        TTaskInfo *task = new TTaskInfo();
        task->RemoteFile = fname;
        task->LocalFile = SaveDialog->FileName;
        ListTask->Items->AddObject("GetFile(" + task->RemoteFile + ")", (TObject*)task);
      }
    }
  }
  else if (cmd == "PutFile" || cmd == "SDPutFile")
  {
    OpenDialog->FileName = "*.*";
    if (OpenDialog->Execute())
    {
      OpenDialog->InitialDir = ExtractFileDir(OpenDialog->FileName);
      for (int i = 0; i < OpenDialog->Files->Count; i ++)
      {
        TTaskInfo *task = new TTaskInfo();
        if (cmd == "SDPutFile")
        {
          AnsiString path = "\\PRIVATE\\AU\\DF";
          if (InputQuery(cmd, "Input path", path) && path != "")
          {
            if (path != "\\")
            {
              if (path[1] == '\\')
                path = path.SubString(2, path.Length() - 1);
              if (path[path.Length()] != '\\')
                path = path + "\\";
            }
            task->RemoteFile = path + ExtractFileName(OpenDialog->Files->Strings[i]);
          }
          else
          {
            delete task;
            continue;
          }
        }
        else
          task->RemoteFile = ExtractFileName(OpenDialog->Files->Strings[i]);
        task->LocalFile = OpenDialog->Files->Strings[i];
        ListTask->Items->AddObject("PutFile(" + task->RemoteFile + ")", (TObject*)task);
      }
    }
  }
  else if (cmd == "DelFile")
  {
    for (int i = 0; i < ListFile->Items->Count; i ++)
      if (ListFile->Items->Item[i]->Selected)
      {
        TTaskInfo *task = new TTaskInfo();
        task->RemoteFile = ListFile->Items->Item[i]->Caption;
        ListTask->Items->AddObject("DelFile(" + task->RemoteFile + ")", (TObject*)task);
      }
  }
  else if (cmd == "DelFile(InputFileName)")
  {
    AnsiString fname;
    if (InputQuery(cmd, "Input filename", fname) && fname != "")
    {
      TTaskInfo *task = new TTaskInfo();
      task->RemoteFile = fname;
      ListTask->Items->AddObject("DelFile(" + task->RemoteFile + ")", (TObject*)task);
    }
  }
  else if (cmd == "SDGetFileList")
  {
    for (int i = 0; i < ListTask->Items->Count; i ++)
      if (ListTask->Items->Strings[i].Pos("GetFileList") != 0)
      {
        Application->MessageBox("Already exists.", "add task", MB_ICONERROR | MB_OK);
        return;
      }

    AnsiString path = "\\";
    if (InputQuery(cmd, "Input path", path) && path != "")
    {
      if (path != "\\")
      {
        if (path[1] == '\\')
          path = path.SubString(2, path.Length() - 1);
        if (path[path.Length()] == '\\')
          path = path.SubString(1, path.Length() - 1);
      }
      TTaskInfo *task = new TTaskInfo();
      task->RemoteFile = path;
      ListTask->Items->AddObject("SDGetFileList(" + task->RemoteFile + ")", (TObject*)task);
    }
  }
  else
  {
    for (int i = 0; i < ListTask->Items->Count; i ++)
    {
      if (ListTask->Items->Strings[i] == cmd
       || cmd.Pos("GetFileList") != 0 && ListTask->Items->Strings[i].Pos("GetFileList") != 0)
      {
        Application->MessageBox("Already exists.", "add task", MB_ICONERROR | MB_OK);
        return;
      }
    }
    if (cmd == "GetTelephoneNumber" || cmd == "SearchLockNumber")
      ListTask->Items->InsertObject(0, cmd, NULL);
    else
      ListTask->Items->AddObject(cmd, NULL);
  }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::ButtonDelClick(TObject *Sender)
{
  if (MyThread)
    return;
  for (int i = 0; i < ListTask->Items->Count; i ++)
    if (ListTask->Selected[i])
    {
      if (ListTask->Items->Objects[i])
        delete (TTaskInfo*)ListTask->Items->Objects[i];
      ListTask->Items->Delete(i);
      i --;
    }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::ButtonUpClick(TObject *Sender)
{
  if (MyThread)
    return;
  for (int i = 1; i < ListTask->Items->Count; i ++)
    if (ListTask->Selected[i]
     && ListTask->Items->Strings[i] != "GetTelephoneNumber" && ListTask->Items->Strings[i] != "SearchLockNumber"
     && ListTask->Items->Strings[i-1] != "GetTelephoneNumber" && ListTask->Items->Strings[i-1] != "SearchLockNumber")
    {
      ListTask->Items->Exchange(i, i-1);
      ListTask->Selected[i-1] = true;
    }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::ButtonDownClick(TObject *Sender)
{
  if (MyThread)
    return;
  for (int i = ListTask->Items->Count-2; i >= 0; i --)
    if (ListTask->Selected[i]
     && ListTask->Items->Strings[i] != "GetTelephoneNumber" && ListTask->Items->Strings[i] != "SearchLockNumber"
     && ListTask->Items->Strings[i+1] != "GetTelephoneNumber" && ListTask->Items->Strings[i+1] != "SearchLockNumber")
    {
      ListTask->Items->Exchange(i, i+1);
      ListTask->Selected[i+1] = true;
    }
}
//---------------------------------------------------------------------------
int CALLBACK BrowseCallback(HWND hwnd, UINT msg, LPARAM lparam, LPARAM data)
{
  switch (msg)
  {
    case BFFM_INITIALIZED:
      SendMessage( hwnd , BFFM_SETSELECTION , true , data);
      break;
    case BFFM_SELCHANGED:
      break;
  }
  return 0;
}
void __fastcall TForm1::KeyGetFileClick(TObject *Sender)
{
  if (MyThread)
    return;

  AnsiString dir = "";
  for (int i = 0; i < ListFile->Items->Count; i ++)
    if (ListFile->Items->Item[i]->Selected)
    {
      if (ListFile->Items->Item[i]->Caption[ListFile->Items->Item[i]->Caption.Length()] == '\\')
      {
        int j;
        for (j = 0; j < ListTask->Items->Count; j ++)
          if (ListTask->Items->Strings[j].Pos("GetFileList") != 0)
            break;
        if (j != ListTask->Items->Count)
          continue;

        TTaskInfo *task = new TTaskInfo();
        if (ListFile->Items->Item[i]->Caption[1] == '\\')
          task->RemoteFile = ListFile->Items->Item[i]->Caption.SubString(2,ListFile->Items->Item[i]->Caption.Length()-2);
        else
          task->RemoteFile = ListFile->Items->Item[i]->Caption.SubString(1,ListFile->Items->Item[i]->Caption.Length()-1);
        ListTask->Items->AddObject("SDGetFileList(" + task->RemoteFile + ")", (TObject*)task);
        continue;
      }

      if (dir == "")
      {
        dir = SaveDialog->InitialDir;
        //if (!SelectDirectory("保存フォルダ", "", dir))
        //  return;
        //if (!SelectDirectory(dir, TSelectDirOpts() << sdAllowCreate << sdPerformCreate << sdPrompt, 0))
        //  return;

        BROWSEINFO bi;
        char path[MAX_PATH];
        char file[MAX_PATH];
        LPITEMIDLIST id;

        memset(&bi, 0, sizeof(BROWSEINFO));
        memset(path, 0, MAX_PATH);
        memset(file, 0, MAX_PATH);

        bi.hwndOwner = Handle;
        bi.pidlRoot = NULL;
        bi.pszDisplayName = file;
        bi.lpszTitle = "保存フォルダを選択して下さい";
        bi.ulFlags = BIF_RETURNONLYFSDIRS;
        bi.lpfn = (BFFCALLBACK)BrowseCallback;
        bi.lParam = (LPARAM)dir.c_str();
        bi.iImage = 0;

        id = SHBrowseForFolder(&bi);
        if (!SHGetPathFromIDList(id, path))
          path[0] = 0;
        GlobalFreePtr(id);

        dir = path;
        if (dir == "")
          return;

        SaveDialog->InitialDir = dir;
        if (dir[dir.Length()] != '\\')
          dir += "\\";
      }

      //SaveDialog->FileName = ListFile->Items->Item[i]->Caption;
      //if (SaveDialog->Execute())
      //{
        //SaveDialog->InitialDir = ExtractFileDir(SaveDialog->FileName);
        TTaskInfo *task = new TTaskInfo();
        task->RemoteFile = ListFile->Items->Item[i]->Caption;
        //task->LocalFile = SaveDialog->FileName;
        task->LocalFile = dir + ExtractFileName(task->RemoteFile);
        ListTask->Items->AddObject("GetFile(" + task->RemoteFile + ")", (TObject*)task);
      //}
    }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::KeyDelFileClick(TObject *Sender)
{
  if (MyThread)
    return;

  for (int i = 0; i < ListFile->Items->Count; i ++)
    if (ListFile->Items->Item[i]->Selected)
    {
      if (ExtractFileDir(ListFile->Items->Item[i]->Caption.Pos("\\")) != "")
        continue;
      TTaskInfo *task = new TTaskInfo();
      task->RemoteFile = ListFile->Items->Item[i]->Caption;
      ListTask->Items->AddObject("DelFile(" + task->RemoteFile + ")", (TObject*)task);
    }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::ListFileDblClick(TObject *Sender)
{
  KeyGetFileClick(Sender);
}
//---------------------------------------------------------------------------
void __fastcall TForm1::ListTaskDblClick(TObject *Sender)
{
  ButtonDelClick(Sender);
}
//---------------------------------------------------------------------------
void __fastcall TForm1::ComboMobileChange(TObject *Sender)
{
  TIniFile *Ini = new TIniFile(IniFile);
  try
  {
    EditBaud->Text = Ini->ReadString(ComboMobile->Text, "BaudRate", EditBaud->Text);
    EditFrame->Text = Ini->ReadString(ComboMobile->Text, "FrameSize", EditFrame->Text);
  }
  __finally
  {
    delete Ini;
  }
}
//---------------------------------------------------------------------------
int CALLBACK CustomSortProc(LPARAM Item1, LPARAM Item2, LPARAM i)
{
  if (i > 1)
  {
    i -= 2;
    int j = CompareText(((TListItem*)Item1)->SubItems->Strings[i], ((TListItem*)Item2)->SubItems->Strings[i]);
    if (j != 0)
      return j;
    i = 1;
  }
  else if (i < -1)
  {
    i = -i - 2;
    int j = -CompareText(((TListItem*)Item1)->SubItems->Strings[i], ((TListItem*)Item2)->SubItems->Strings[i]);
    if (j != 0)
      return j;
    i = -1;
  }
  return i * CompareText(((TListItem*)Item1)->Caption, ((TListItem*)Item2)->Caption);
}
void __fastcall TForm1::ListFileColumnClick(TObject *Sender,
      TListColumn *Column)
{
  int i = Column->Index + 1;
  ListFile->Tag = ((ListFile->Tag != i) ? i : -i);
  ListFile->CustomSort(CustomSortProc, ListFile->Tag);
}
//---------------------------------------------------------------------------

