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
    }
    function screenshots() {
        controls.screenshots();
    }
    function next() {
        controls.next();
    }
    function cover() {
        controls.cover();
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
    
    function show() {
        var info = video.info
        tagList.tags = Qt.binding(function() { return info.tags; })
        screenshot.screenshots = Qt.binding(function() { return info.screenshots; })
        seekbar.bookmarks = Qt.binding(function() { return info.bookmarks; })
        controls.bookmarks = Qt.binding(function() { return info.bookmarks; })
        toolbar.model = info.files

        videoPlayer.opacity = 1
        videoPlayer.x = Qt.binding(function() { return mainView.x; })
        videoPlayer.y = Qt.binding(function() { return mainView.y; })
        videoPlayer.width = Qt.binding(function() { return mainView.width })
        videoPlayer.height = Qt.binding(function() { return mainView.height })
        focus = true


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
        videoModel.setLastFile(gridView.currentIndex, file)
        videoModel.setLastPosition(gridView.currentIndex, mediaPlayer.position)
        gridView.focus = true
        videoPlayer.opacity = 0
    }
    
    VideoOutput {
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
    
    MouseArea {
        id: videoMouseArea
        propagateComposedEvents: true
        hoverEnabled: parent.visible
        enabled: hoverEnabled
        anchors.fill: parent

        
        onMouseXChanged: {
            if (mouse.y > parent.height - seekbar.height){
                seekbar.opacity = 1
                screenshot.position = mouse.x
                //         screenshot.y = mouse.y
            }else if(mouse.y < toolbar.height)
                toolbar.opacity = 1
            else if (mouse.x < tagList.width)
                tagList.opacity = 1
            else if (mouse.x > parent.width - 200) {
                controls.opacity = 1
            } else {
                toolbar.opacity = 0
                seekbar.opacity = 0
                tagList.opacity = 0
                controls.opacity = 0
                return false;
            }
        }
        onClicked: {
            if (mouse.x > 200 && mouse.x < width - controls.width && mouse.y < parent.height - seekbar.height && mouse.y > parent.width - toolbar.width) {
                parent.hide()
            } else {
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
            errorText.text = "Error while playing file " + source + ": " + errorString
        }
    }
    
    Screenshot {
        id: screenshot
        duration: mediaPlayer.duration
        path: parent.path
        file: parent.file
        screenshots: []
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
    }
    
    ControlBar {
        id: controls
        anchors.verticalCenter: parent.verticalCenter
        anchors.right: parent.right
        player: mediaPlayer
        index: parent.index
        file: parent.file
    }
    
    Toolbar {
        id: toolbar
        model: []
        onFileChanged: {
            parent.position = 0
            parent.file = toolbar.file
        }
        videoName: parent.name
        folderPath: parent.path
        file: parent.file
    }
    
    TagList {
        id: tagList
        tags: []
        anchors.top: toolbar.bottom
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
        //        onStopped: screenshot.position = Qt.binding(function() { return mediaPlayer.position; })
    }
}
