pragma Singleton

import QtQuick

QtObject {
    property QtObject color: QtObject {
        readonly property color transparent: 'transparent'

        readonly property color textPrimary: '#FAFAFA'
        readonly property color textSecondary: '#E4E4E7'
        readonly property color textTertiary: '#A1A1AA'
        readonly property color textQuaternary: '#86868F'
        readonly property color textInverted: '#09090B'
        readonly property color textAccent: '#93C5FD'
        readonly property color textStaticWhite: '#FFFFFF'

        readonly property color bgBase: '#101012'
        readonly property color bgSurface: '#18181B'
        readonly property color bgSurfaceAlt: '#1F1F23'
        readonly property color bgSheet: '#313136'
        readonly property color bgPanel: '#3F3F46'
        readonly property color bgPanelAlt: '#52525B'
        readonly property color bgOverlay: '#8009090B'
        readonly property color bgStateLayer: '#14FAFAFA'
        readonly property color bgStateLayerPressed: '#1AFAFAFA'
        readonly property color bgStateLayerAccent: '#1493C5FD'
        readonly property color bgSurfaceInverse: '#E4E4E7'
        readonly property color bgSurfaceInverseAlt: '#D4D4D8'
        readonly property color bgControlTrack: '#1F1F23'
        readonly property color bgField: '#1F1F23'
        readonly property color bgSelected: '#1F1F23'
        readonly property color bgAccentSubtle: '#4D2563EB'
        readonly property color bgStaticWhite: '#FFFFFF'

        readonly property color bgAction: '#E4E4E7'
        readonly property color bgActionHover: '#D4D4D8'
        readonly property color bgActionPress: '#A1A1AA'

        readonly property color borderFaint: '#313136'
        readonly property color borderSoft: '#3F3F46'
        readonly property color borderMedium: '#52525B'
        readonly property color borderBold: '#A1A1AA'
        readonly property color borderEmphasis: '#E4E4E7'
        readonly property color borderAccent: '#93C5FD'
        readonly property color borderField: '#52525B'
        readonly property color borderFocus: '#E4E4E7'

        readonly property color accentLight: '#93C5FD'
        readonly property color accentDefault: '#3B82F6'
        readonly property color accentDark: '#2563EB'
        readonly property color accentDeep: '#1E3A8A'

        readonly property color statusDestructive: '#F87171'
        readonly property color statusWarning: '#EAB308'
        readonly property color statusSuccess: '#4ADE80'

        readonly property color promoRecommendation: '#A78BFA'
        readonly property color promoUpdate: '#38BDF8'
        readonly property color promoOverlay: '#148B5CF6'
        readonly property color promoPink: '#DB2777'
        readonly property color promoPinkDeep: '#9D174D'

        readonly property color iconDefault: '#E4E4E7'
        readonly property color iconSecondary: '#A1A1AA'
        readonly property color iconTertiary: '#3F3F46'
        readonly property color iconInverted: '#09090B'

        readonly property color shadowLayer1: '#26000000'
        readonly property color shadowLayer2: '#4D000000'
        readonly property color shadowGlow: '#60A5FA'
    }
}
