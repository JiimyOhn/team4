object Progressform: TProgressform
  Left = 0
  Top = 0
  Caption = 'RawConnection'
  ClientHeight = 141
  ClientWidth = 676
  Color = clBtnFace
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -11
  Font.Name = 'Tahoma'
  Font.Style = []
  Position = poScreenCenter
  TextHeight = 13
  object ProgressLabel: TLabel
    Left = 24
    Top = 20
    Width = 67
    Height = 13
    Alignment = taCenter
    Caption = 'In Progress...'
  end
  object ProgressBar1: TProgressBar
    Left = 24
    Top = 45
    Width = 625
    Height = 17
    TabOrder = 0
  end
  object Cancel: TButton
    Left = 288
    Top = 88
    Width = 75
    Height = 25
    Caption = 'Cancel'
    TabOrder = 1
    OnClick = CancelClick
  end
end
