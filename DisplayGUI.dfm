object Form1: TForm1
  Left = 0
  Top = 0
  Caption = 'ADS-B Viewer'
  ClientHeight = 1041
  ClientWidth = 1228
  Color = clBtnFace
  Constraints.MinHeight = 740
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -10
  Font.Name = 'Tahoma'
  Font.Style = [fsBold]
  Menu = MainMenu1
  Position = poScreenCenter
  WindowState = wsMaximized
  OnMouseWheel = FormMouseWheel
  TextHeight = 12
  object Label16: TLabel
    Left = 13
    Top = 67
    Width = 31
    Height = 13
    Caption = 'ICAO:'
    Font.Charset = DEFAULT_CHARSET
    Font.Color = clWindowText
    Font.Height = -11
    Font.Name = 'Tahoma'
    Font.Style = [fsBold]
    ParentFont = False
  end
  object Label17: TLabel
    Left = 48
    Top = 67
    Width = 46
    Height = 13
    Caption = 'No Hook'
    Font.Charset = DEFAULT_CHARSET
    Font.Color = clWindowText
    Font.Height = -11
    Font.Name = 'Tahoma'
    Font.Style = [fsBold]
    ParentFont = False
  end
  object RightPanel: TPanel
    Left = 978
    Top = 0
    Width = 250
    Height = 1041
    Align = alRight
    Color = clWhite
    Constraints.MaxWidth = 250
    DoubleBuffered = True
    ParentBackground = False
    ParentDoubleBuffered = False
    TabOrder = 0
    object RightPanelTabControl: TPageControl
      Left = 1
      Top = 1
      Width = 248
      Height = 892
      ActivePage = AircraftInfoTab
      Align = alTop
      TabOrder = 0
      object AircraftInfoTab: TTabSheet
        Caption = 'Info'
        object PlaybackSpeedLabel: TLabel
          Left = 8
          Top = 464
          Width = 92
          Height = 13
          Caption = 'Playback Speed:'
          Font.Charset = DEFAULT_CHARSET
          Font.Color = clWindowText
          Font.Height = -11
          Font.Name = 'Tahoma'
          Font.Style = [fsBold]
          ParentFont = False
        end
        object Panel5: TPanel
          Left = 3
          Top = 3
          Width = 234
          Height = 77
          BevelInner = bvLowered
          BevelKind = bkFlat
          TabOrder = 0
          object Lon: TLabel
            Left = 31
            Top = 40
            Width = 7
            Height = 13
            Caption = '0'
            Font.Charset = DEFAULT_CHARSET
            Font.Color = clWindowText
            Font.Height = -11
            Font.Name = 'Tahoma'
            Font.Style = [fsBold]
            ParentFont = False
          end
          object Label3: TLabel
            Left = 5
            Top = 40
            Width = 23
            Height = 13
            Caption = 'Lon:'
            Font.Charset = DEFAULT_CHARSET
            Font.Color = clWindowText
            Font.Height = -11
            Font.Name = 'Tahoma'
            Font.Style = [fsBold]
            ParentFont = False
          end
          object Lat: TLabel
            Left = 31
            Top = 21
            Width = 7
            Height = 13
            Caption = '0'
            Font.Charset = DEFAULT_CHARSET
            Font.Color = clWindowText
            Font.Height = -11
            Font.Name = 'Tahoma'
            Font.Style = [fsBold]
            ParentFont = False
          end
          object Label2: TLabel
            Left = 5
            Top = 21
            Width = 21
            Height = 13
            Caption = 'Lat:'
            Font.Charset = DEFAULT_CHARSET
            Font.Color = clWindowText
            Font.Height = -11
            Font.Name = 'Tahoma'
            Font.Style = [fsBold]
            ParentFont = False
          end
          object SystemTimeLabel: TLabel
            Left = 5
            Top = 4
            Width = 74
            Height = 13
            Caption = 'System Time'
            Font.Charset = DEFAULT_CHARSET
            Font.Color = clWindowText
            Font.Height = -11
            Font.Name = 'Tahoma'
            Font.Style = [fsBold]
            ParentFont = False
          end
          object ViewableAircraftCountLabel: TLabel
            Left = 157
            Top = 59
            Width = 7
            Height = 12
            Caption = '0'
          end
          object AircraftCountLabel: TLabel
            Left = 66
            Top = 58
            Width = 7
            Height = 13
            Caption = '0'
            Font.Charset = DEFAULT_CHARSET
            Font.Color = clWindowText
            Font.Height = -11
            Font.Name = 'Tahoma'
            Font.Style = [fsBold]
            ParentFont = False
          end
          object Label11: TLabel
            Left = 101
            Top = 58
            Width = 53
            Height = 13
            Caption = 'Viewable:'
            Font.Charset = DEFAULT_CHARSET
            Font.Color = clWindowText
            Font.Height = -11
            Font.Name = 'Tahoma'
            Font.Style = [fsBold]
            ParentFont = False
          end
          object Label1: TLabel
            Left = 5
            Top = 58
            Width = 58
            Height = 13
            Caption = '# Aircraft:'
            Font.Charset = DEFAULT_CHARSET
            Font.Color = clWindowText
            Font.Height = -11
            Font.Name = 'Tahoma'
            Font.Style = [fsBold]
            ParentFont = False
          end
          object SystemTime: TStaticText
            Left = 85
            Top = 4
            Width = 76
            Height = 17
            Caption = '00:00:00:000'
            Font.Charset = DEFAULT_CHARSET
            Font.Color = clWindowText
            Font.Height = -11
            Font.Name = 'Tahoma'
            Font.Style = [fsBold]
            ParentFont = False
            TabOrder = 0
          end
        end
        object Panel4: TPanel
          Left = 3
          Top = 82
          Width = 234
          Height = 348
          BevelInner = bvLowered
          BevelKind = bkFlat
          TabOrder = 1
          object CLatLabel: TLabel
            Left = 66
            Top = 57
            Width = 21
            Height = 13
            Caption = 'N/A'
            Font.Charset = DEFAULT_CHARSET
            Font.Color = clWindowText
            Font.Height = -11
            Font.Name = 'Tahoma'
            Font.Style = [fsBold]
            ParentFont = False
          end
          object CLonLabel: TLabel
            Left = 66
            Top = 76
            Width = 21
            Height = 13
            Caption = 'N/A'
            Font.Charset = DEFAULT_CHARSET
            Font.Color = clWindowText
            Font.Height = -11
            Font.Name = 'Tahoma'
            Font.Style = [fsBold]
            ParentFont = False
            Transparent = False
          end
          object SpdLabel: TLabel
            Left = 66
            Top = 92
            Width = 21
            Height = 13
            Caption = 'N/A'
            Font.Charset = DEFAULT_CHARSET
            Font.Color = clWindowText
            Font.Height = -11
            Font.Name = 'Tahoma'
            Font.Style = [fsBold]
            ParentFont = False
          end
          object HdgLabel: TLabel
            Left = 66
            Top = 111
            Width = 21
            Height = 13
            Caption = 'N/A'
            Font.Charset = DEFAULT_CHARSET
            Font.Color = clWindowText
            Font.Height = -11
            Font.Name = 'Tahoma'
            Font.Style = [fsBold]
            ParentFont = False
          end
          object AltLabel: TLabel
            Left = 66
            Top = 130
            Width = 21
            Height = 13
            Caption = 'N/A'
            Font.Charset = DEFAULT_CHARSET
            Font.Color = clWindowText
            Font.Height = -11
            Font.Name = 'Tahoma'
            Font.Style = [fsBold]
            ParentFont = False
          end
          object MsgCntLabel: TLabel
            Left = 66
            Top = 149
            Width = 21
            Height = 13
            Caption = 'N/A'
            Font.Charset = DEFAULT_CHARSET
            Font.Color = clWindowText
            Font.Height = -11
            Font.Name = 'Tahoma'
            Font.Style = [fsBold]
            ParentFont = False
          end
          object TrkLastUpdateTimeLabel: TLabel
            Left = 120
            Top = 171
            Width = 72
            Height = 13
            Caption = '00:00:00:000'
            Font.Charset = DEFAULT_CHARSET
            Font.Color = clWindowText
            Font.Height = -11
            Font.Name = 'Tahoma'
            Font.Style = [fsBold]
            ParentFont = False
          end
          object Label14: TLabel
            Left = 5
            Top = 172
            Width = 109
            Height = 13
            Caption = 'LAST UPDATE TIME:'
            Font.Charset = DEFAULT_CHARSET
            Font.Color = clWindowText
            Font.Height = -11
            Font.Name = 'Tahoma'
            Font.Style = [fsBold]
            ParentFont = False
          end
          object Label13: TLabel
            Left = 5
            Top = 150
            Width = 52
            Height = 13
            Caption = 'MSG CNT:'
            Font.Charset = DEFAULT_CHARSET
            Font.Color = clWindowText
            Font.Height = -11
            Font.Name = 'Tahoma'
            Font.Style = [fsBold]
            ParentFont = False
          end
          object Label10: TLabel
            Left = 5
            Top = 131
            Width = 24
            Height = 13
            Caption = 'ALT:'
            Font.Charset = DEFAULT_CHARSET
            Font.Color = clWindowText
            Font.Height = -11
            Font.Name = 'Tahoma'
            Font.Style = [fsBold]
            ParentFont = False
          end
          object Label9: TLabel
            Left = 5
            Top = 112
            Width = 27
            Height = 13
            Caption = 'HDG:'
            Font.Charset = DEFAULT_CHARSET
            Font.Color = clWindowText
            Font.Height = -11
            Font.Name = 'Tahoma'
            Font.Style = [fsBold]
            ParentFont = False
          end
          object Label8: TLabel
            Left = 5
            Top = 93
            Width = 25
            Height = 13
            Caption = 'SPD:'
            Font.Charset = DEFAULT_CHARSET
            Font.Color = clWindowText
            Font.Height = -11
            Font.Name = 'Tahoma'
            Font.Style = [fsBold]
            ParentFont = False
          end
          object Label7: TLabel
            Left = 5
            Top = 76
            Width = 24
            Height = 13
            Caption = 'LON:'
            Font.Charset = DEFAULT_CHARSET
            Font.Color = clWindowText
            Font.Height = -11
            Font.Name = 'Tahoma'
            Font.Style = [fsBold]
            ParentFont = False
          end
          object Label6: TLabel
            Left = 5
            Top = 57
            Width = 24
            Height = 13
            Caption = 'LAT:'
            Font.Charset = DEFAULT_CHARSET
            Font.Color = clWindowText
            Font.Height = -11
            Font.Name = 'Tahoma'
            Font.Style = [fsBold]
            ParentFont = False
          end
          object Label18: TLabel
            Left = 5
            Top = 38
            Width = 55
            Height = 13
            Caption = 'FLIGHT #:'
            Font.Charset = DEFAULT_CHARSET
            Font.Color = clWindowText
            Font.Height = -11
            Font.Name = 'Tahoma'
            Font.Style = [fsBold]
            ParentFont = False
          end
          object FlightNumLabel: TLabel
            Left = 66
            Top = 38
            Width = 21
            Height = 13
            Caption = 'N/A'
            Font.Charset = DEFAULT_CHARSET
            Font.Color = clWindowText
            Font.Height = -11
            Font.Name = 'Tahoma'
            Font.Style = [fsBold]
            ParentFont = False
          end
          object ICAOLabel: TLabel
            Left = 66
            Top = 22
            Width = 46
            Height = 13
            Caption = 'No Hook'
            Font.Charset = DEFAULT_CHARSET
            Font.Color = clWindowText
            Font.Height = -11
            Font.Name = 'Tahoma'
            Font.Style = [fsBold]
            ParentFont = False
          end
          object Label5: TLabel
            Left = 5
            Top = 19
            Width = 31
            Height = 13
            Caption = 'ICAO:'
            Font.Charset = DEFAULT_CHARSET
            Font.Color = clWindowText
            Font.Height = -11
            Font.Name = 'Tahoma'
            Font.Style = [fsBold]
            ParentFont = False
          end
          object LabelAirlineName: TLabel
            Left = 5
            Top = 193
            Width = 39
            Height = 13
            Caption = 'Airline:'
            Font.Charset = DEFAULT_CHARSET
            Font.Color = clWindowText
            Font.Height = -11
            Font.Name = 'Tahoma'
            Font.Style = [fsBold]
            ParentFont = False
          end
          object AirlineNameLabel: TLabel
            Left = 65
            Top = 192
            Width = 21
            Height = 13
            Caption = 'N/A'
            Font.Charset = DEFAULT_CHARSET
            Font.Color = clWindowText
            Font.Height = -11
            Font.Name = 'Tahoma'
            Font.Style = [fsBold]
            ParentFont = False
          end
          object LabelAirlineCountry: TLabel
            Left = 5
            Top = 214
            Width = 48
            Height = 13
            Caption = 'Country:'
            Font.Charset = DEFAULT_CHARSET
            Font.Color = clWindowText
            Font.Height = -11
            Font.Name = 'Tahoma'
            Font.Style = [fsBold]
            ParentFont = False
          end
          object AirlineCountryLabel: TLabel
            Left = 65
            Top = 215
            Width = 21
            Height = 13
            Caption = 'N/A'
            Font.Charset = DEFAULT_CHARSET
            Font.Color = clWindowText
            Font.Height = -11
            Font.Name = 'Tahoma'
            Font.Style = [fsBold]
            ParentFont = False
          end
          object LabelModel: TLabel
            Left = 5
            Top = 235
            Width = 49
            Height = 13
            Caption = 'AC Type:'
            Font.Charset = DEFAULT_CHARSET
            Font.Color = clWindowText
            Font.Height = -11
            Font.Name = 'Tahoma'
            Font.Style = [fsBold]
            ParentFont = False
          end
          object AircraftModelLabel: TLabel
            Left = 65
            Top = 235
            Width = 21
            Height = 13
            Caption = 'N/A'
            Font.Charset = DEFAULT_CHARSET
            Font.Color = clWindowText
            Font.Height = -11
            Font.Name = 'Tahoma'
            Font.Style = [fsBold]
            ParentFont = False
          end
          object Label4: TLabel
            Left = 75
            Top = 0
            Width = 85
            Height = 16
            Caption = 'Close Control'
            Font.Charset = DEFAULT_CHARSET
            Font.Color = clWindowText
            Font.Height = -13
            Font.Name = 'Tahoma'
            Font.Style = [fsBold, fsUnderline]
            ParentFont = False
          end
          object Label20: TLabel
            Left = 5
            Top = 255
            Width = 40
            Height = 13
            Caption = 'ROUTE:'
            Font.Charset = DEFAULT_CHARSET
            Font.Color = clWindowText
            Font.Height = -11
            Font.Name = 'Tahoma'
            Font.Style = [fsBold]
            ParentFont = False
          end
          object RouteInfoMemo: TMemo
            Left = 66
            Top = 253
            Width = 150
            Height = 69
            TabStop = False
            BorderStyle = bsNone
            Font.Charset = DEFAULT_CHARSET
            Font.Color = clWindowText
            Font.Height = -11
            Font.Name = 'Tahoma'
            Font.Style = [fsBold]
            Lines.Strings = (
              'N/A')
            ParentFont = False
            ParentShowHint = False
            ReadOnly = True
            ShowHint = True
            TabOrder = 0
          end
        end
        object BigQueryCheckBox: TCheckBox
          Left = 8
          Top = 436
          Width = 124
          Height = 17
          Caption = 'BigQuery Upload'
          TabOrder = 2
          OnClick = BigQueryCheckBoxClick
        end
        object PlaybackSpeedComboBox: TComboBox
          Left = 100
          Top = 461
          Width = 60
          Height = 20
          Style = csDropDownList
          ItemIndex = 0
          TabOrder = 3
          Text = '1x'
          Items.Strings = (
            '1x'
            '2x'
            '3x')
        end
        object Panel2: TPanel
          Left = 3
          Top = 490
          Width = 234
          Height = 147
          BevelInner = bvLowered
          BevelKind = bkFlat
          Color = clWhite
          TabOrder = 4
          object Label12: TLabel
            Left = 8
            Top = 8
            Width = 55
            Height = 12
            Caption = 'CPA TIME:'
          end
          object Label19: TLabel
            Left = 8
            Top = 26
            Width = 82
            Height = 12
            Caption = 'CPA DISTANCE:'
          end
          object CpaTimeValue: TLabel
            Left = 69
            Top = 8
            Width = 28
            Height = 12
            Caption = 'NONE'
          end
          object CpaDistanceValue: TLabel
            Left = 96
            Top = 26
            Width = 25
            Height = 12
            Caption = 'None'
          end
          object ZoomIn: TButton
            Left = 5
            Top = 110
            Width = 65
            Height = 18
            Caption = 'Zoom In'
            Font.Charset = DEFAULT_CHARSET
            Font.Color = clWindowText
            Font.Height = -11
            Font.Name = 'Tahoma'
            Font.Style = [fsBold]
            ParentFont = False
            TabOrder = 0
            OnClick = ZoomInClick
          end
          object ZoomOut: TButton
            Left = 160
            Top = 110
            Width = 65
            Height = 18
            Caption = 'Zoom Out'
            Font.Charset = DEFAULT_CHARSET
            Font.Color = clWindowText
            Font.Height = -11
            Font.Name = 'Tahoma'
            Font.Style = [fsBold]
            ParentFont = False
            TabOrder = 1
            OnClick = ZoomOutClick
          end
          object TimeToGoTrackBar: TTrackBar
            Left = 5
            Top = 71
            Width = 228
            Height = 33
            Max = 1800
            Min = 1
            Frequency = 100
            Position = 1
            TabOrder = 2
            OnChange = TimeToGoTrackBarChange
          end
          object TimeToGoCheckBox: TCheckBox
            Left = 8
            Top = 38
            Width = 105
            Height = 27
            Caption = 'Time-To-Go'
            Checked = True
            State = cbChecked
            TabOrder = 3
          end
          object TimeToGoText: TStaticText
            Left = 119
            Top = 43
            Width = 76
            Height = 16
            Caption = '00:00:00:000'
            TabOrder = 4
          end
        end
      end
      object ControlsFiltersTab: TTabSheet
        Caption = 'Filter'
        ImageIndex = 1
        object Label28: TLabel
          Left = 80
          Top = 70
          Width = 80
          Height = 14
          Caption = 'Aircraft Filter'
          Font.Charset = DEFAULT_CHARSET
          Font.Color = clWindowText
          Font.Height = -12
          Font.Name = 'Tahoma'
          Font.Style = [fsBold, fsUnderline]
          ParentFont = False
        end
        object Label25: TLabel
          Left = 24
          Top = 91
          Width = 36
          Height = 13
          Caption = 'Airline'
          Font.Charset = DEFAULT_CHARSET
          Font.Color = clWindowText
          Font.Height = -11
          Font.Name = 'Tahoma'
          Font.Style = [fsBold]
          ParentFont = False
        end
        object Label26: TLabel
          Left = 104
          Top = 91
          Width = 33
          Height = 13
          Caption = 'Origin'
          Font.Charset = DEFAULT_CHARSET
          Font.Color = clWindowText
          Font.Height = -11
          Font.Name = 'Tahoma'
          Font.Style = [fsBold]
          ParentFont = False
        end
        object Label27: TLabel
          Left = 168
          Top = 91
          Width = 65
          Height = 13
          Caption = 'Destination'
          Font.Charset = DEFAULT_CHARSET
          Font.Color = clWindowText
          Font.Height = -11
          Font.Name = 'Tahoma'
          Font.Style = [fsBold]
          ParentFont = False
        end
        object Label15: TLabel
          Left = 80
          Top = 140
          Width = 98
          Height = 13
          Caption = 'Areas Of Interest'
          Font.Charset = DEFAULT_CHARSET
          Font.Color = clWindowText
          Font.Height = -11
          Font.Name = 'Tahoma'
          Font.Style = [fsBold]
          ParentFont = False
        end
        object SpeedFilterLabel: TLabel
          Left = 5
          Top = 365
          Width = 126
          Height = 13
          Caption = 'Speed: 0 ~ 1000 knots'
          Font.Charset = DEFAULT_CHARSET
          Font.Color = clWindowText
          Font.Height = -11
          Font.Name = 'Tahoma'
          Font.Style = [fsBold]
          ParentFont = False
        end
        object AltitudeFilterLabel: TLabel
          Left = 5
          Top = 425
          Width = 120
          Height = 13
          Caption = 'Altitude: 0 ~ 60000 ft'
          Font.Charset = DEFAULT_CHARSET
          Font.Color = clWindowText
          Font.Height = -11
          Font.Name = 'Tahoma'
          Font.Style = [fsBold]
          ParentFont = False
        end
        object AircraftCategoryLabel: TLabel
          Left = 5
          Top = 485
          Width = 101
          Height = 13
          Caption = 'Aircraft Category:'
          Font.Charset = DEFAULT_CHARSET
          Font.Color = clWindowText
          Font.Height = -11
          Font.Name = 'Tahoma'
          Font.Style = [fsBold]
          ParentFont = False
        end
        object DrawMap: TCheckBox
          Left = 5
          Top = 10
          Width = 96
          Height = 17
          Caption = 'Display Map'
          Checked = True
          Font.Charset = DEFAULT_CHARSET
          Font.Color = clWindowText
          Font.Height = -11
          Font.Name = 'Tahoma'
          Font.Style = [fsBold]
          ParentFont = False
          State = cbChecked
          TabOrder = 0
        end
        object PurgeStale: TCheckBox
          Left = 5
          Top = 33
          Width = 96
          Height = 19
          Caption = 'Purge Stale'
          Checked = True
          Font.Charset = DEFAULT_CHARSET
          Font.Color = clWindowText
          Font.Height = -11
          Font.Name = 'Tahoma'
          Font.Style = [fsBold]
          ParentFont = False
          State = cbChecked
          TabOrder = 1
        end
        object CycleImages: TCheckBox
          Left = 107
          Top = 33
          Width = 96
          Height = 19
          Caption = 'Cycle Images'
          Font.Charset = DEFAULT_CHARSET
          Font.Color = clWindowText
          Font.Height = -11
          Font.Name = 'Tahoma'
          Font.Style = [fsBold]
          ParentFont = False
          TabOrder = 2
          OnClick = CycleImagesClick
        end
        object CSpinStaleTime: TCSpinEdit
          Left = 107
          Top = 10
          Width = 62
          Height = 21
          Increment = 5
          MaxValue = 1000
          MinValue = 5
          TabOrder = 3
          Value = 90
        end
        object PurgeButton: TButton
          Left = 186
          Top = 10
          Width = 51
          Height = 16
          Caption = 'Purge'
          TabOrder = 4
          OnClick = PurgeButtonClick
        end
        object FilterOriginEdit: TEdit
          Left = 95
          Top = 106
          Width = 57
          Height = 20
          TabOrder = 5
          OnChange = FilterOriginEditChange
        end
        object FilterAirlineEdit: TEdit
          Left = 16
          Top = 106
          Width = 57
          Height = 20
          TabOrder = 6
          OnChange = FilterAirlineEditChange
        end
        object FilterDestinationEdit: TEdit
          Left = 174
          Top = 106
          Width = 57
          Height = 20
          TabOrder = 7
          OnChange = FilterDestinationEditChange
        end
        object AreaListView: TListView
          Left = 5
          Top = 160
          Width = 228
          Height = 120
          Columns = <
            item
              Caption = 'Area'
              Width = 170
            end
            item
              Caption = 'Color'
              Width = 40
            end>
          ReadOnly = True
          RowSelect = True
          TabOrder = 8
          ViewStyle = vsReport
          OnCustomDrawItem = AreaListViewCustomDrawItem
          OnSelectItem = AreaListViewSelectItem
        end
        object Insert: TButton
          Left = 5
          Top = 286
          Width = 36
          Height = 18
          Caption = 'Insert'
          TabOrder = 9
          OnClick = InsertClick
        end
        object Delete: TButton
          Left = 157
          Top = 286
          Width = 44
          Height = 18
          Caption = 'Delete'
          Enabled = False
          TabOrder = 10
          OnClick = DeleteClick
        end
        object Complete: TButton
          Left = 47
          Top = 286
          Width = 57
          Height = 18
          Caption = 'Complete'
          Enabled = False
          TabOrder = 11
          OnClick = CompleteClick
        end
        object Cancel: TButton
          Left = 110
          Top = 286
          Width = 41
          Height = 18
          Caption = 'Cancel'
          Enabled = False
          TabOrder = 12
          OnClick = CancelClick
        end
        object FilterPolygonOnlyCheckBox: TCheckBox
          Left = 5
          Top = 310
          Width = 150
          Height = 17
          Caption = 'Polygon area only'
          Font.Charset = DEFAULT_CHARSET
          Font.Color = clWindowText
          Font.Height = -11
          Font.Name = 'Tahoma'
          Font.Style = [fsBold]
          ParentFont = False
          TabOrder = 13
          OnClick = FilterPolygonOnlyCheckBoxClick
        end
        object SpeedMinTrackBar: TTrackBar
          Left = 5
          Top = 385
          Width = 110
          Height = 30
          Max = 1000
          Frequency = 50
          TabOrder = 14
          OnChange = SpeedFilterTrackBarChange
        end
        object SpeedMaxTrackBar: TTrackBar
          Left = 125
          Top = 385
          Width = 110
          Height = 30
          Max = 1000
          Frequency = 50
          Position = 1000
          TabOrder = 15
          OnChange = SpeedFilterTrackBarChange
        end
        object AltitudeMinTrackBar: TTrackBar
          Left = 5
          Top = 445
          Width = 110
          Height = 30
          Max = 60000
          Frequency = 2000
          TabOrder = 16
          OnChange = AltitudeFilterTrackBarChange
        end
        object AltitudeMaxTrackBar: TTrackBar
          Left = 125
          Top = 445
          Width = 110
          Height = 30
          Max = 60000
          Frequency = 2000
          Position = 60000
          TabOrder = 17
          OnChange = AltitudeFilterTrackBarChange
        end
        object CommercialCheckBox: TCheckBox
          Left = 5
          Top = 510
          Width = 80
          Height = 17
          Caption = 'Commercial'
          Checked = True
          State = cbChecked
          TabOrder = 18
          OnClick = AircraftCategoryFilterChange
        end
        object CargoCheckBox: TCheckBox
          Left = 95
          Top = 510
          Width = 60
          Height = 17
          Caption = 'Cargo'
          Checked = True
          State = cbChecked
          TabOrder = 19
          OnClick = AircraftCategoryFilterChange
        end
        object HelicopterCheckBox: TCheckBox
          Left = 165
          Top = 510
          Width = 80
          Height = 17
          Caption = 'Helicopter'
          Checked = True
          State = cbChecked
          TabOrder = 20
          OnClick = AircraftCategoryFilterChange
        end
        object MilitaryCheckBox: TCheckBox
          Left = 5
          Top = 535
          Width = 60
          Height = 17
          Caption = 'Military'
          Checked = True
          State = cbChecked
          TabOrder = 21
          OnClick = AircraftCategoryFilterChange
        end
        object GeneralAviationCheckBox: TCheckBox
          Left = 75
          Top = 535
          Width = 100
          Height = 17
          Caption = 'General Aviation'
          Checked = True
          State = cbChecked
          TabOrder = 22
          OnClick = AircraftCategoryFilterChange
        end
        object BusinessJetCheckBox: TCheckBox
          Left = 5
          Top = 560
          Width = 80
          Height = 17
          Caption = 'Business Jet'
          Checked = True
          State = cbChecked
          TabOrder = 23
          OnClick = AircraftCategoryFilterChange
        end
        object GliderCheckBox: TCheckBox
          Left = 95
          Top = 560
          Width = 60
          Height = 17
          Caption = 'Glider'
          Checked = True
          State = cbChecked
          TabOrder = 25
          OnClick = AircraftCategoryFilterChange
        end
        object UltralightCheckBox: TCheckBox
          Left = 165
          Top = 560
          Width = 80
          Height = 17
          Caption = 'Ultralight'
          Checked = True
          State = cbChecked
          TabOrder = 24
          OnClick = AircraftCategoryFilterChange
        end
      end
      object DataSourcesTab: TTabSheet
        Caption = 'Data source'
        ImageIndex = 2
        object Label16_MapSettings: TLabel
          Left = 5
          Top = 246
          Width = 74
          Height = 13
          Caption = 'Map Settings'
          Font.Charset = DEFAULT_CHARSET
          Font.Color = clWindowText
          Font.Height = -11
          Font.Name = 'Tahoma'
          Font.Style = [fsBold]
          ParentFont = False
        end
        object RawConnectButton: TButton
          Left = 5
          Top = 10
          Width = 104
          Height = 18
          Caption = 'Raw Connect'
          TabOrder = 0
          OnClick = RawConnectButtonClick
        end
        object RawIpAddress: TEdit
          Left = 115
          Top = 10
          Width = 110
          Height = 20
          TabOrder = 1
          Text = '127.0.0.1'
        end
        object RawPlaybackButton: TButton
          Left = 133
          Top = 34
          Width = 96
          Height = 17
          Caption = 'Raw Playback'
          TabOrder = 2
          OnClick = RawPlaybackButtonClick
        end
        object RawRecordButton: TButton
          Left = 5
          Top = 34
          Width = 104
          Height = 17
          Caption = 'Raw Record'
          TabOrder = 3
          OnClick = RawRecordButtonClick
        end
        object SBSConnectButton: TButton
          Left = 5
          Top = 57
          Width = 104
          Height = 20
          Caption = 'SBS Connect'
          TabOrder = 4
          OnClick = SBSConnectButtonClick
        end
        object SBSIpAddress: TEdit
          Left = 115
          Top = 57
          Width = 117
          Height = 20
          TabOrder = 5
          Text = 'data.adsbhub.org'
        end
        object SBSRecordButton: TButton
          Left = 5
          Top = 83
          Width = 104
          Height = 17
          Caption = 'SBS Record'
          TabOrder = 6
          OnClick = SBSRecordButtonClick
        end
        object SBSPlaybackButton: TButton
          Left = 115
          Top = 83
          Width = 96
          Height = 17
          Caption = 'SBS Playback'
          TabOrder = 7
          OnClick = SBSPlaybackButtonClick
        end
        object Panel6: TPanel
          Left = 5
          Top = 106
          Width = 234
          Height = 103
          TabOrder = 8
          object Label21: TLabel
            Left = 56
            Top = 8
            Width = 119
            Height = 16
            Caption = 'Connection Status'
            Font.Charset = DEFAULT_CHARSET
            Font.Color = clWindowText
            Font.Height = -13
            Font.Name = 'Tahoma'
            Font.Style = [fsBold]
            ParentFont = False
          end
          object Label22: TLabel
            Left = 5
            Top = 40
            Width = 76
            Height = 13
            Caption = 'Raw Connect:'
            Font.Charset = DEFAULT_CHARSET
            Font.Color = clWindowText
            Font.Height = -11
            Font.Name = 'Tahoma'
            Font.Style = [fsBold]
            ParentFont = False
          end
          object Label23: TLabel
            Left = 5
            Top = 59
            Width = 64
            Height = 13
            Caption = 'SBS Status:'
            Font.Charset = DEFAULT_CHARSET
            Font.Color = clWindowText
            Font.Height = -11
            Font.Name = 'Tahoma'
            Font.Style = [fsBold]
            ParentFont = False
          end
          object Label24: TLabel
            Left = 5
            Top = 78
            Width = 125
            Height = 13
            Caption = 'API Last Update Time:'
            Font.Charset = DEFAULT_CHARSET
            Font.Color = clWindowText
            Font.Height = -11
            Font.Name = 'Tahoma'
            Font.Style = [fsBold]
            ParentFont = False
          end
          object RawConnectStatus: TLabel
            Left = 87
            Top = 40
            Width = 82
            Height = 13
            Caption = 'Not Connected'
            Font.Charset = DEFAULT_CHARSET
            Font.Color = clWindowText
            Font.Height = -11
            Font.Name = 'Tahoma'
            Font.Style = [fsBold]
            ParentFont = False
          end
          object SBSConnectStatus: TLabel
            Left = 87
            Top = 59
            Width = 82
            Height = 13
            Caption = 'Not Connected'
            Font.Charset = DEFAULT_CHARSET
            Font.Color = clWindowText
            Font.Height = -11
            Font.Name = 'Tahoma'
            Font.Style = [fsBold]
            ParentFont = False
          end
          object APILastUpdateTime: TLabel
            Left = 136
            Top = 78
            Width = 65
            Height = 13
            Caption = '00:00:00:00'
            Font.Charset = DEFAULT_CHARSET
            Font.Color = clWindowText
            Font.Height = -11
            Font.Name = 'Tahoma'
            Font.Style = [fsBold]
            ParentFont = False
          end
        end
        object MapComboBox: TComboBox
          Left = 5
          Top = 220
          Width = 100
          Height = 20
          Style = csDropDownList
          TabOrder = 9
          OnChange = MapComboBoxChange
          Items.Strings = (
            'Google Maps'
            'VFR'
            'IFS Low'
            'IFS High'
            'OpenStreetMap')
        end
        object Panel7: TPanel
          Left = 5
          Top = 270
          Width = 234
          Height = 290
          BevelInner = bvLowered
          BevelKind = bkFlat
          TabOrder = 10
          object Label29: TLabel
            Left = 75
            Top = 8
            Width = 84
            Height = 16
            Caption = 'Conflict Filter'
            Font.Charset = DEFAULT_CHARSET
            Font.Color = clWindowText
            Font.Height = -13
            Font.Name = 'Tahoma'
            Font.Style = [fsBold, fsUnderline]
            ParentFont = False
          end
          object Label30: TLabel
            Left = 5
            Top = 35
            Width = 91
            Height = 13
            Caption = 'TCPA Threshold:'
            Font.Charset = DEFAULT_CHARSET
            Font.Color = clWindowText
            Font.Height = -11
            Font.Name = 'Tahoma'
            Font.Style = [fsBold]
            ParentFont = False
          end
          object Label31: TLabel
            Left = 5
            Top = 95
            Width = 113
            Height = 13
            Caption = 'Horizontal Distance:'
            Font.Charset = DEFAULT_CHARSET
            Font.Color = clWindowText
            Font.Height = -11
            Font.Name = 'Tahoma'
            Font.Style = [fsBold]
            ParentFont = False
          end
          object Label32: TLabel
            Left = 5
            Top = 155
            Width = 98
            Height = 13
            Caption = 'Vertical Distance:'
            Font.Charset = DEFAULT_CHARSET
            Font.Color = clWindowText
            Font.Height = -11
            Font.Name = 'Tahoma'
            Font.Style = [fsBold]
            ParentFont = False
          end
          object TCPAThresholdLabel: TLabel
            Left = 5
            Top = 75
            Width = 100
            Height = 13
            Caption = 'TCPA: 0 ~ 300 sec'
            Font.Charset = DEFAULT_CHARSET
            Font.Color = clWindowText
            Font.Height = -11
            Font.Name = 'Tahoma'
            Font.Style = [fsBold]
            ParentFont = False
          end
          object HorizontalDistanceLabel: TLabel
            Left = 5
            Top = 135
            Width = 95
            Height = 13
            Caption = 'H Dist: 0 ~ 10 NM'
            Font.Charset = DEFAULT_CHARSET
            Font.Color = clWindowText
            Font.Height = -11
            Font.Name = 'Tahoma'
            Font.Style = [fsBold]
            ParentFont = False
          end
          object VerticalDistanceLabel: TLabel
            Left = 5
            Top = 195
            Width = 100
            Height = 13
            Caption = 'V Dist: 0 ~ 5000 ft'
            Font.Charset = DEFAULT_CHARSET
            Font.Color = clWindowText
            Font.Height = -11
            Font.Name = 'Tahoma'
            Font.Style = [fsBold]
            ParentFont = False
          end
          object TCPAMinTrackBar: TTrackBar
            Left = 5
            Top = 50
            Width = 110
            Height = 20
            Max = 900
            Frequency = 90
            Position = 10
            TabOrder = 0
            OnChange = TCPAFilterTrackBarChange
          end
          object TCPAMaxTrackBar: TTrackBar
            Left = 125
            Top = 50
            Width = 110
            Height = 20
            Max = 900
            Frequency = 90
            Position = 900
            TabOrder = 1
            OnChange = TCPAFilterTrackBarChange
          end
          object HorizontalMaxTrackBar: TTrackBar
            Left = 5
            Top = 110
            Width = 225
            Height = 20
            Max = 50
            Frequency = 5
            Position = 10
            TabOrder = 2
            OnChange = HorizontalDistanceFilterTrackBarChange
          end
          object VerticalMaxTrackBar: TTrackBar
            Left = 5
            Top = 170
            Width = 225
            Height = 20
            Max = 3000
            Frequency = 300
            Position = 1000
            TabOrder = 3
            OnChange = VerticalDistanceFilterTrackBarChange
          end
          object ShowConflictAircraftAlwaysCheckBox: TCheckBox
            Left = 5
            Top = 220
            Width = 220
            Height = 17
            Caption = 'Show conflict aircraft always'
            Font.Charset = DEFAULT_CHARSET
            Font.Color = clWindowText
            Font.Height = -11
            Font.Name = 'Tahoma'
            Font.Style = [fsBold]
            ParentFont = False
            TabOrder = 4
            OnClick = ShowConflictAircraftAlwaysCheckBoxClick
          end
          object ShowOnlyConflictAircraftCheckBox: TCheckBox
            Left = 5
            Top = 240
            Width = 220
            Height = 17
            Caption = 'Show only conflict aircraft'
            Font.Charset = DEFAULT_CHARSET
            Font.Color = clWindowText
            Font.Height = -11
            Font.Name = 'Tahoma'
            Font.Style = [fsBold]
            ParentFont = False
            TabOrder = 5
            OnClick = ShowOnlyConflictAircraftCheckBoxClick
          end
        end
        object DeadReckoningCheckBox: TCheckBox
          Left = 16
          Top = 576
          Width = 185
          Height = 17
          Caption = 'DeadReckoning'
          TabOrder = 11
        end
      end
    end
  end
  object ObjectDisplay: TOpenGLPanel
    Left = 0
    Top = 0
    Width = 978
    Height = 1041
    Cursor = crCross
    Align = alClient
    TabOrder = 1
    PFDFlags = [f_PFD_DRAW_TO_WINDOW, f_PFD_SUPPORT_OPENGL, f_PFD_DOUBLEBUFFER]
    Font3D_Type.Charset = ANSI_CHARSET
    Font3D_Type.Color = clWindowText
    Font3D_Type.Height = -9
    Font3D_Type.Name = 'Arial'
    Font3D_Type.Style = []
    Font3D_Extrustion = 0.100000001490116100
    Font2D_Enabled = True
    Font2D_Type.Charset = ANSI_CHARSET
    Font2D_Type.Color = clWindowText
    Font2D_Type.Height = -27
    Font2D_Type.Name = 'Arial'
    Font2D_Type.Style = [fsBold]
    OnMouseDown = ObjectDisplayMouseDown
    OnMouseMove = ObjectDisplayMouseMove
    OnMouseUp = ObjectDisplayMouseUp
    OnInit = ObjectDisplayInit
    OnResize = ObjectDisplayResize
    OnPaint = ObjectDisplayPaint
    object DeviationLabel: TLabel
      Left = 8
      Top = 128
      Width = 131
      Height = 13
      Caption = 'Route Deviation Alerts:'
      Font.Charset = DEFAULT_CHARSET
      Font.Color = clRed
      Font.Height = -11
      Font.Name = 'Tahoma'
      Font.Style = [fsBold]
      ParentFont = False
    end
    object ConflictListView: TListView
      Left = 0
      Top = 0
      Width = 362
      Height = 120
      Columns = <
        item
          Caption = 'Aircraft 1'
          Width = 65
        end
        item
          Caption = 'Aircraft 2'
          Width = 65
        end
        item
          Caption = 'TCPA'
        end
        item
          Caption = 'H Dist'
        end
        item
          Caption = 'V Dist'
        end
        item
          Caption = 'Risk'
          Width = 60
        end>
      GridLines = True
      TabOrder = 0
      ViewStyle = vsReport
    end
    object DeviationListView: TListView
      Left = 0
      Top = 148
      Width = 380
      Height = 80
      Columns = <
        item
          Caption = 'Aircraft'
          Width = 80
        end
        item
          Caption = 'Altitude'
          Width = 80
        end
        item
          Caption = 'Latitude'
          Width = 100
        end
        item
          Caption = 'Longitude'
          Width = 100
        end>
      GridLines = True
      TabOrder = 1
      ViewStyle = vsReport
    end
  end
  object MainMenu1: TMainMenu
    Left = 24
    object File1: TMenuItem
      Caption = 'File'
      object UseSBSRemote: TMenuItem
        Caption = 'ADS-B Hub'
        OnClick = UseSBSRemoteClick
      end
      object UseSBSLocal: TMenuItem
        Caption = 'ADS-B Local'
        OnClick = UseSBSLocalClick
      end
      object LoadARTCCBoundaries1: TMenuItem
        Caption = 'Load ARTCC Boundaries'
        OnClick = LoadARTCCBoundaries1Click
      end
      object Exit1: TMenuItem
        Caption = 'Exit'
        OnClick = Exit1Click
      end
    end
  end
  object Timer1: TTimer
    Interval = 500
    OnTimer = Timer1Timer
    Left = 88
  end
  object Timer2: TTimer
    Interval = 5000
    OnTimer = Timer2Timer
    Left = 136
  end
  object AssessmentTimer: TTimer
    Interval = 5000
    OnTimer = AssessmentTimerTimer
    Left = 208
  end
  object RecordRawSaveDialog: TSaveDialog
    DefaultExt = 'raw'
    Filter = 'raw|*.raw'
    Left = 328
  end
  object PlaybackRawDialog: TOpenDialog
    DefaultExt = 'raw'
    Filter = 'raw|*.raw'
    Left = 448
  end
  object RecordSBSSaveDialog: TSaveDialog
    DefaultExt = 'sbs'
    Filter = 'sbs|*.sbs'
    Left = 664
  end
  object PlaybackSBSDialog: TOpenDialog
    DefaultExt = 'sbs'
    Filter = 'sbs|*.sbs'
    Left = 784
  end
end
