import QtQuick 2.0

Item {
    id: scrollBar

    property real position: view.visibleArea.yPosition
    property real pageSize: view.visibleArea.heightRatio
    //property alias bgColor: background.color
    property alias fgColor: thumb.color
    property var view
    opacity: 0.7
    anchors.right: parent.right
    anchors.bottom: parent.bottom
    anchors.top: parent.top
    width: 20
    smooth: true

    MouseArea {
        anchors.fill: parent
        onMouseYChanged: {
            if (mouse.buttons & Qt.LeftButton) {
                var pos = mouse.y / height - parent.pageSize/2
                if (pos < 0) pos = 0
                if (pos + parent.pageSize/2 > 1) pos = 1 - parent.pageSize/2
                view.contentY = pos * view.contentHeight
            }
        }
    }

    // A light, semi-transparent background
    /*Rectangle {
        id: background
        color: "white";
        anchors.fill: parent
    }*/
    // Size the bar to the required size, depending upon the orientation.
    Rectangle {
        id: thumb
        color: "white"
        radius: width/2 - 1
        x: 1
        y: scrollBar.position * (scrollBar.height-2) + 1
        width: parent.width-2
        height: scrollBar.pageSize * (scrollBar.height-2)
        smooth: true
    }
}
