object Form1: TForm1
  Left = 192
  Top = 107
  AutoScroll = False
  Caption = 'KTrans'
  ClientHeight = 300
  ClientWidth = 460
  Color = clBtnFace
  Font.Charset = SHIFTJIS_CHARSET
  Font.Color = clWindowText
  Font.Height = -12
  Font.Name = 'ＭＳ Ｐゴシック'
  Font.Style = []
  OldCreateOrder = False
  Position = poScreenCenter
  OnCloseQuery = FormCloseQuery
  OnCreate = FormCreate
  OnDestroy = FormDestroy
  PixelsPerInch = 96
  TextHeight = 12
  object Splitter1: TSplitter
    Left = 160
    Top = 30
    Width = 3
    Height = 270
    Cursor = crHSplit
    AutoSnap = False
    Beveled = True
    MinSize = 50
    ResizeStyle = rsUpdate
  end
  object PanelTop: TPanel
    Left = 0
    Top = 0
    Width = 460
    Height = 30
    Align = alTop
    BevelOuter = bvNone
    TabOrder = 0
    OnResize = PanelTopResize
    object ButtonClear: TButton
      Left = 410
      Top = 5
      Width = 45
      Height = 20
      Caption = 'Clea&r'
      TabOrder = 6
      OnClick = ButtonClearClick
    end
    object ComboCable: TComboBox
      Left = 70
      Top = 5
      Width = 65
      Height = 20
      ItemHeight = 12
      TabOrder = 1
      Items.Strings = (
        'Serial'
        'Modem1'
        'Modem2'
        'Modem3')
    end
    object ComboPort: TComboBox
      Left = 5
      Top = 5
      Width = 60
      Height = 20
      ItemHeight = 12
      TabOrder = 0
      Text = 'COM1'
      Items.Strings = (
        'COM1'
        'COM2'
        'COM3'
        'COM4'
        'COM5'
        'COM6'
        'COM7'
        'COM8'
        'COM9')
    end
    object ButtonConnect: TButton
      Left = 325
      Top = 5
      Width = 70
      Height = 20
      Caption = '&Connect'
      TabOrder = 5
      OnClick = ButtonConnectClick
    end
    object EditFrame: TEdit
      Left = 270
      Top = 5
      Width = 50
      Height = 20
      ImeMode = imDisable
      TabOrder = 4
      Text = '0x400'
    end
    object EditBaud: TEdit
      Left = 220
      Top = 5
      Width = 50
      Height = 20
      ImeMode = imDisable
      TabOrder = 3
      Text = '115200'
    end
    object ComboMobile: TComboBox
      Left = 140
      Top = 5
      Width = 75
      Height = 20
      Style = csDropDownList
      ItemHeight = 12
      TabOrder = 2
      OnChange = ComboMobileChange
      Items.Strings = (
        'A3012CA'
        'A5303H')
    end
  end
  object PanelMain: TPanel
    Left = 163
    Top = 30
    Width = 297
    Height = 270
    Align = alClient
    BevelOuter = bvNone
    TabOrder = 2
    object Splitter2: TSplitter
      Left = 0
      Top = 85
      Width = 297
      Height = 3
      Cursor = crVSplit
      Align = alTop
      AutoSnap = False
      Beveled = True
      MinSize = 75
      ResizeStyle = rsUpdate
    end
    object Log: TRichEdit
      Left = 0
      Top = 0
      Width = 297
      Height = 85
      Align = alTop
      ImeMode = imDisable
      PlainText = True
      ReadOnly = True
      ScrollBars = ssVertical
      TabOrder = 0
      OnChange = LogChange
    end
    object PanelBottom: TPanel
      Left = 0
      Top = 88
      Width = 297
      Height = 50
      Align = alTop
      BevelOuter = bvNone
      TabOrder = 1
      OnResize = PanelBottomResize
      object LabelProgress: TLabel
        Left = 5
        Top = 7
        Width = 100
        Height = 12
        Alignment = taRightJustify
        AutoSize = False
        Caption = '0/0'
      end
      object ComboCmd: TComboBox
        Left = 110
        Top = 25
        Width = 140
        Height = 20
        Style = csDropDownList
        ItemHeight = 12
        TabOrder = 3
        Items.Strings = (
          'GetTelephoneNumber'
          'SearchLockNumber'
          'GetProfile(0000h)'
          'GetProfile(0100h)'
          'GetFileList'
          'GetFile'
          'GetFile(InputFileName)'
          'PutFile'
          'DelFile'
          'DelFile(InputFileName)'
          'SDGetFileList'
          'SDPutFile')
      end
      object ButtonAdd: TButton
        Left = 250
        Top = 25
        Width = 45
        Height = 20
        Caption = '&Add'
        TabOrder = 4
        OnClick = ButtonAddClick
      end
      object ButtonDown: TButton
        Left = 75
        Top = 25
        Width = 30
        Height = 20
        Caption = '↓'
        TabOrder = 2
        OnClick = ButtonDownClick
      end
      object ButtonDel: TButton
        Left = 40
        Top = 25
        Width = 30
        Height = 20
        Caption = '&DEL'
        TabOrder = 1
        OnClick = ButtonDelClick
      end
      object ButtonUp: TButton
        Left = 5
        Top = 25
        Width = 30
        Height = 20
        Caption = '↑'
        TabOrder = 0
        OnClick = ButtonUpClick
      end
      object ProgressBar1: TProgressBar
        Left = 110
        Top = 5
        Width = 90
        Height = 15
        Min = 0
        Max = 100
        Smooth = True
        TabOrder = 5
      end
      object ProgressBar2: TProgressBar
        Left = 205
        Top = 5
        Width = 90
        Height = 15
        Min = 0
        Max = 100
        Smooth = True
        TabOrder = 6
      end
    end
    object ListTask: TListBox
      Left = 0
      Top = 138
      Width = 297
      Height = 132
      Align = alClient
      ItemHeight = 12
      MultiSelect = True
      TabOrder = 2
      OnDblClick = ListTaskDblClick
    end
  end
  object ListFile: TListView
    Left = 0
    Top = 30
    Width = 160
    Height = 270
    Align = alLeft
    Columns = <
      item
        Caption = 'ファイル名'
        Width = 90
      end
      item
        Caption = '拡張子'
      end>
    FullDrag = True
    HideSelection = False
    LargeImages = IconListSmall
    MultiSelect = True
    ReadOnly = True
    RowSelect = True
    PopupMenu = PopupMenu1
    SmallImages = IconListSmall
    TabOrder = 1
    ViewStyle = vsReport
    OnColumnClick = ListFileColumnClick
    OnDblClick = ListFileDblClick
  end
  object OpenDialog: TOpenDialog
    Filter = 'すべてのファイル(*.*)|*.*'
    Options = [ofHideReadOnly, ofAllowMultiSelect, ofPathMustExist, ofFileMustExist, ofEnableSizing]
    Title = 'ファイルを選択'
    Left = 10
    Top = 40
  end
  object PopupMenu1: TPopupMenu
    AutoHotkeys = maManual
    Left = 10
    Top = 110
    object KeyGetFile: TMenuItem
      Caption = '&GetFile'
      Default = True
      OnClick = KeyGetFileClick
    end
    object KeyDelFile: TMenuItem
      Caption = '&DelFile'
      OnClick = KeyDelFileClick
    end
  end
  object SaveDialog: TSaveDialog
    Filter = 'すべてのファイル(*.*)|*.*'
    Options = [ofOverwritePrompt, ofHideReadOnly, ofPathMustExist, ofNoReadOnlyReturn, ofEnableSizing]
    Title = 'ファイルに保存'
    Left = 10
    Top = 75
  end
  object IconListSmall: TImageList
    Left = 10
    Top = 145
  end
end
