import QtQuick.Controls
import QtQuick
import QtQuick.Layouts

Item {
    id: root
    width: 400
    height: 600
    visible: true
    property alias shaderModel: view.model

    signal editoShader(string theShader)
    signal removeShader(int rowIndex)
    signal changeInputId(int rowIndex,int newValue)



    Menu
    {
        id:menu
        property int viewIndex
        MenuItem
        {
            id:changerShader
            text:"编辑shader"
            onTriggered: {
                root.editoShader(view.model[menu.viewIndex].path)
            }
        }

        MenuItem
        {
            id: removeShader
            text: "删除shader"
            onTriggered: {
                 root.removeShader(menu.viewIndex)
                }
        }

    }

    // 黑色背景
    Rectangle { anchors.fill: parent; color: "#222" }

    ListView {
        id: view
        anchors.fill: parent
        clip: true

        model:root.shaderModel

        delegate: Item {


            MouseArea
            {
                anchors.fill: parent
                acceptedButtons: Qt.RightButton

                onClicked:
                {
                    menu.viewIndex=index
                    menu.popup()
                }
            }

            width: ListView.view.width

            height: 40

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 10
                anchors.rightMargin: 10
                spacing: 10

                Text {
                    text: index+"."+modelData.path.toString().split("/").pop()

                    font.pixelSize: 14
                    color: "white"

                    Layout.fillWidth: true

                    Layout.alignment: Qt.AlignVCenter
                    elide: Text.ElideMiddle
                }



                Text {
                    text: "输入源:"
                    font.pixelSize: 12
                    color: "#AAA"
                    Layout.alignment: Qt.AlignVCenter
                }

                ComboBox {

                    Layout.preferredWidth: 60
                    Layout.preferredHeight: 28
                    Layout.alignment: Qt.AlignVCenter
                    model: {
                        var arr = []
                        for(var i=0; i<view.count; i++) arr.push(i)
                        arr.push(-1)
                        return arr
                    }

                    currentIndex:modelData.inputId

                    onActivated:(selectionIndex)=>
                                {
                                    root.changeInputId(index,selectionIndex)
                                }


                    font.pixelSize: 12
                }
            }
        }
    }




}
