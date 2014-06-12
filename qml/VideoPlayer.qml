import QtQuick 2.0
import QtGraphicalEffects 1.0
import QtMultimedia 5.0


Rectangle {
    id: videoPlayer

    property var video: gridView.currentItem

    x: video == null ? 0 : video.x + gridView.x - gridView.contentX
    y: video == null ? 0 : video.y + gridView.y - gridView.contentY
    color: "black"
    height: video == null ? 0 : video.height
    width: video == null ? 0 :video.width
    property int position: 0
    property string file: ""
    onFileChanged: {
        var info = video.info
        mediaPlayer.source = "file:" + encodeURIComponent(info.path + "/" + file)
        mediaPlayer.seek(position)
        mediaPlayer.play()
        screenshotOverview.filterScreenshots()
    }
    property string path: ""
    property string name: ""
    property int index: -1
    opacity: 0
    visible: false
    onOpacityChanged: {
        if (opacity == 0) { visible = false; mediaPlayer.pause() } else { visible = true }
        if (opacity == 1) mediaPlayer.play()
    }

    Behavior on height { NumberAnimation { duration: 300; }}
    Behavior on width { NumberAnimation { duration: 300; }}
    Behavior on x { NumberAnimation { duration: 300; }}
    Behavior on y { NumberAnimation { duration: 300; }}
    Behavior on opacity { NumberAnimation { duration: 300; }}

    function setPlaybackSpeed(speed) {
        mediaPlayer.playbackRate = speed
    }

    function hideAllScreenshots() {
        screenshotOverview.opacity = 0
    }

    function showAllScreenshots() {
        screenshotOverview.opacity = 1
    }

    function seek(seekAmount) {
        var newPosition = mediaPlayer.position + seekAmount
        if (newPosition > mediaPlayer.duration) {
            newPosition %= mediaPlayer.duration
        }
        if (newPosition < 0) {
            newPosition = mediaPlayer.duration + newPosition
        }

        mediaPlayer.seek(newPosition)
        screenshot.position = mediaPlayer.position * seekbar.width / mediaPlayer.duration
        seekbarpeek.restart()
    }

    function bookmark() {
        controls.bookmark();
        seekbarpeek.restart()
    }
    function takeScreenshots() {
        controls.takeScreenshots();
    }
    function next() {
        controls.next();
        seekbarpeek.restart()
    }
    function cover() {
        controls.createCover();
    }
    function tags() {
        tagList.opacity = 1
    }
    function togglePause() {
        if (mediaPlayer.playbackState === MediaPlayer.PausedState) {
            mediaPlayer.play()
        } else {
            mediaPlayer.pause()
        }
        seekbarpeek.restart()
    }
    function escapePressed() {
        if (screenshotOverview.opacity == 1) {
            screenshotOverview.opacity = 0
            videoMouseArea.forceActiveFocus()
            cursorTimer.start()
            videoMouseArea.cursorShape = Qt.BlankCursor
        } else {
            hide()
        }
    }
    
    function show() {
        var info = video.info
        tagList.tags = Qt.binding(function() { return info.tags; })
        screenshot.screenshots = Qt.binding(function() { return info.screenshots; })
        seekbar.bookmarks = Qt.binding(function() { return info.bookmarks; })
        controls.bookmarks = Qt.binding(function() { return info.bookmarks; })
        controls.description = Qt.binding(function() { return info.description; })
        controls.cover = "file:/" + encodeURIComponent(info.coverPath)
        toolbar.model = info.files

        videoPlayer.opacity = 1
        videoPlayer.x = Qt.binding(function() { return mainView.x; })
        videoPlayer.y = Qt.binding(function() { return mainView.y; })
        videoPlayer.width = Qt.binding(function() { return mainView.width })
        videoPlayer.height = Qt.binding(function() { return mainView.height })
        focus = true

        cursorTimer.restart()

        videoMouseArea.forceActiveFocus()

        tagList.filterTagList()

        file = info.lastFile
        position = info.lastPosition
        path = info.path
        name = info.name
        index = info.index
    }

    function hide() {
        videoPlayer.x = Qt.binding(function() { return video.x + gridView.x - gridView.contentX; })
        videoPlayer.y = Qt.binding(function() { return video.y + gridView.y - gridView.contentY; })
        videoPlayer.width = Qt.binding(function() { return video.width })
        videoPlayer.height = Qt.binding(function() { return video.height })
        tagList.tags = []
        screenshot.screenshots = []
        seekbar.bookmarks = []
        controls.bookmarks = []
        controls.cover = ""
        controls.description = ""

        videoModel.setLastFile(gridView.currentIndex, file)
        videoModel.setLastPosition(gridView.currentIndex, mediaPlayer.position)
        gridView.focus = true
        videoPlayer.opacity = 0
    }
    
    VideoOutput {
        id: videoOutput
        anchors.fill: parent
        source: mediaPlayer
        
        Text {
            id: errorText
            anchors.fill: parent
            visible: false
            color: "red"
            font.pointSize: 20
            font.bold: true
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            wrapMode: TextInput.WordWrap
        }
    }
/*
    ShaderEffect {
        onLogChanged: console.log(log)
        property variant source: ShaderEffectSource { sourceItem: videoOutput; hideSource: true }
        property real dividerValue: 1
        property real bright: 1.1
        property real contrast: 1.1
        property real saturation: 1.1
        anchors.fill: videoOutput

        fragmentShader: "
uniform sampler2D source;
uniform lowp float qt_Opacity;
uniform lowp float bright;
uniform lowp float contrast;
uniform lowp float saturation;
varying vec2 v_TexCoordinate;
varying vec2 qt_TexCoord0;
const vec3 W = vec3(0.2125, 0.7154, 0.0721);

vec3 BrightnessContrastSaturation(vec3 color, float brt, float con, float sat)
{
    vec3 black = vec3(0., 0., 0.);
    vec3 middle = vec3(0.5, 0.5, 0.5);
    float luminance = dot(color, W);
    vec3 gray = vec3(luminance, luminance, luminance);

    vec3 brtColor = mix(black, color, brt);
    vec3 conColor = mix(middle, brtColor, con);
    vec3 satColor = mix(gray, conColor, sat);
    return satColor;
}


void main()
{
     //get the pixel
     vec2 uv = qt_TexCoord0.xy;
     vec2 st = v_TexCoordinate.st;
     vec3 irgb = texture2D(source, uv).rgb;

     //adjust the brightness/contrast/saturation
     vec3 bcs_result = BrightnessContrastSaturation(irgb, bright, contrast, saturation);

     //add blue
     vec3 blue_result = vec3(bcs_result.r, bcs_result.g*1.03, bcs_result.b);

    float cr = pow(0.1, 2.0);
    float pt = pow(uv.x - 0.5, 2.0) + pow(uv.y - 0.5, 2.0);
    float d = pt - cr;
    float cf = 1.0;
    if (d > 0.0)
        cf = 1.0 - 2.0 * d;
    vec3 col = cf * blue_result.rgb;

     //gl_FragColor = vec4(after_filter, 1.);
     gl_FragColor = qt_Opacity * vec4(blue_result, 1.0);
}"
    }*/
    
    MouseArea {
        id: videoMouseArea
        propagateComposedEvents: true
        hoverEnabled: parent.visible
        enabled: hoverEnabled
        anchors.fill: parent

        onPositionChanged: {
            cursorShape = Qt.ArrowCursor
            cursorTimer.restart()
            if (screenshotOverview.visible) return
            if (mouse.y > parent.height - seekbar.height){
                seekbar.opacity = 1
                screenshot.position = mouse.x
            } else {
                seekbar.opacity = 0
                if (mouse.x > parent.width - 200)
                    controls.opacity = 1
                else
                    controls.opacity = 0
            }

            if (mouse.y < toolbar.height)
                toolbar.opacity = 1
            else
                toolbar.opacity = 0

            if (mouse.x < tagList.width)
                tagList.opacity = 1
            else
                tagList.opacity = 0

        }
        onClicked: {
            if (mouse.x > 200 && mouse.x < width - controls.width && mouse.y < parent.height - seekbar.height && mouse.y > parent.width - toolbar.width) {
                parent.hide()
            }
        }
    }
    
    MediaPlayer {
        id: mediaPlayer
        
        onPlaying: {
            errorText.visible = false
        }
        onStatusChanged: {
            if (status == MediaPlayer.EndOfMedia) {
                seek(0)
                play()
            }
        }
        
        onError: {
            errorText.visible = true
            errorText.text = "Error while playing file: " + errorString
        }
    }
    
    Screenshot {
        id: screenshot
        duration: mediaPlayer.duration
        path: parent.path
        file: parent.file
        screenshots: []
        onScreenshotsChanged: {
            screenshotOverview.filterScreenshots()
        }

        anchors.bottom: seekbar.top
        position: mediaPlayer.position
        visible: false
    }

    RadialGradient {
        id: mask
        verticalRadius: screenshot.height - 50
        horizontalRadius: screenshot.width
        anchors.fill: screenshot
        visible:false
        gradient: Gradient {
            GradientStop { position: 0.3; color: "white" }
            GradientStop { position: 0.5; color: "transparent" }
        }
    }


    OpacityMask {
        id: screenshotMask
        source: screenshot
        maskSource: mask
        anchors.fill: screenshot

        opacity: seekbar.opacity
    }

    SeekBar {
        id: seekbar
        position: mediaPlayer.position
        duration: mediaPlayer.duration
        file: parent.file
        bookmarks: []
        index: parent.index
        onPositionChanged: videoModel.setLastPosition(gridView.currentIndex, mediaPlayer.position)
    }
    
    ControlBar {
        id: controls
        videoName: parent.name
        //anchors.verticalCenter: parent.verticalCenter
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: seekbar.top
        player: mediaPlayer
        index: parent.index
        file: parent.file
        folderPath: parent.path
    }
    
    Toolbar {
        id: toolbar
        anchors.right: controls.left
        anchors.left: tagList.right
        model: []
        onFileChanged: {
            parent.position = 0
            parent.file = toolbar.file
            videoModel.setLastFile(gridView.currentIndex, parent.file)
        }
        folderPath: parent.path
        file: parent.file
        screenshots: screenshot.screenshots
    }
    
    TagList {
        id: tagList
        tags: []
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.bottom: seekbar.top
        index: parent.index
    }

    NumberAnimation {
        id: seekbarpeek
        target: seekbar;
        property: "opacity";
        duration: 500;
        easing.type: Easing.InOutQuad
        from: 1
        to: 0
        running: false
    }

    BusyWidget {
        id: playerBusyWidget
        visible: videoModel.creatingScreenshots
        color: "#99000000"
    }

    Timer {
        id: cursorTimer
        onTriggered: {
            videoMouseArea.cursorShape = Qt.BlankCursor
        }
    }


    GridView {
        id: screenshotOverview
        anchors.fill: parent
        model: []
        interactive: false

        property int position: mediaPlayer.position
        onPositionChanged: {
            var shots = model
            if (shots === undefined) return;

            var position = mediaPlayer.position
            var lastScreenshotPos = -1000000000
            var lastIndex = -1

            for (var i=0; i<shots.length; i++) {
                var screenshotPos = shots[i].split("_")[1]
                if (screenshotPos > position) {
                    if (lastIndex !== -1){
                        currentIndex = lastIndex
                    } else {
                        currentIndex = i
                    }
                    return
                }

                lastScreenshotPos = screenshotPos
                lastIndex = i
            }
        }

        highlight: Rectangle {
            height: parent === null ? 0 : parent.cellHeight + 5
            width: parent === null ? 0 : parent.cellWidth + 5
        }
        highlightFollowsCurrentItem: true

        function filterScreenshots() {
            var fileScreenshots = [];
            var screenshots = screenshot.screenshots
            if (screenshots === undefined) return;

            for (var i=0; i<screenshots.length; i++) {
                if (screenshots[i].indexOf(file) === -1) continue;
                fileScreenshots.push(screenshots[i]);
            }
            model = fileScreenshots
        }

        Behavior on opacity { NumberAnimation { duration: 100; } }
        function resizeScreenshots() {
            var n = model.length
            if (n === 0) return

            var px = Math.ceil(Math.sqrt(n * width / height))
            var sx = 0
            var sy = 0

            if (Math.floor(px * height / width) * px < n)
                sx = height / Math.ceil(px * height / width)
            else
                sx = width / px

            var py = Math.ceil(Math.sqrt(n * height / width))
            if (Math.floor(py * width / height) * py < n)
                sy = width / Math.ceil(py * width / height)
            else
                sy = height / py;

            cellWidth = Math.max(sy, sx)
        }
        onWidthChanged: resizeScreenshots()
        onHeightChanged: resizeScreenshots()
        onCountChanged: resizeScreenshots()

        cellHeight: cellWidth

        opacity: 0
        visible: opacity != 0
        delegate: Item {
            height: screenshotOverview.cellHeight
            width: screenshotOverview.cellWidth
            Image {
                height: screenshotOverview.cellHeight - 3
                width: screenshotOverview.cellWidth - 3
                anchors.centerIn: parent
                id: tinyScreenshot
                asynchronous: true
                opacity: 0.9
                fillMode: Image.PreserveAspectCrop
                source: "file:" + encodeURIComponent(screenshot.path + "/" + modelData)
                MouseArea {
                    cursorShape: screenshotOverview.opacity == 0 ? Qt.BlankCursor : Qt.PointingHandCursor
                    anchors.fill: parent
                    hoverEnabled: false
                    onEntered: {
                        tinyScreenshot.opacity = 1
                    }
                    onExited: {
                        tinyScreenshot.opacity = 0.9
                    }
                    onClicked: {
                        var screenshotPos = modelData.split("_")[1]
                        var tmp = modelData
                        tmp = tmp.substring(tmp.indexOf("_") + 1)
                        tmp = tmp.substring(tmp.indexOf("_") + 1)
                        tmp = tmp.substring(0, tmp.length - 4)
                        videoPlayer.file = tmp
                        mediaPlayer.seek(screenshotPos)
                    }
                }

                Behavior on opacity { NumberAnimation { duration: 100; } }
                Behavior on width { NumberAnimation { duration: 100; } }
                Behavior on height { NumberAnimation { duration: 100; } }
            }
        }
    }
}

