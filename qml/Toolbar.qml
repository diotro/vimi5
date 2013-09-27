import QtQuick 2.0
Rectangle {
    id: toolbar
    height: 0
    anchors.top: parent.top
    anchors.left: parent.left
    anchors.right: parent.right
    color: "black"
    state: "hidden"
    property string video: ""
    property var model
    property int maxHeight: fileList.count * 20 + 20
    property string videoName

    Text {
        anchors.right: parent.right
        anchors.top: parent.top
        color: "white"
        text: parent.videoName
        font.bold: true
    }


    ListView {
        id: fileList
        anchors.fill: parent
        //height: 200
        model: parent.model

        delegate: Text {
            id: fileEntry
            color: "white"
            font.bold: player.file == modelData
            font.pixelSize: 15
            text: " ● " + modelData
            elide: Text.ElideRight
            MouseArea {
                hoverEnabled: true
                anchors.fill: parent
                onClicked: {
                    video = modelData

                }
                cursorShape: Qt.PointingHandCursor
                onEntered: parent.width = rect.width
                onExited: parent.width = fileList.width
            }

        }
    }
    states: [
        State {
            name: "shown"
            PropertyChanges {
                target: toolbar
                height: maxHeight
                opacity: 0.75
            }
        },
        State {
            name: "hidden"
            PropertyChanges {
                target: toolbar
                height: 0
                opacity: 0
            }
        }
    ]

    Behavior on opacity { NumberAnimation { duration: 1000 } }
    Behavior on height { SmoothedAnimation { duration: 1000 } }
}

