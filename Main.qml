import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Dialogs
import MyRhi 1.0
import QtQuick.Layouts

Window {

    id:windwo
    height: Screen.desktopAvailableHeight * 0.75
    width: height * (16.0 / 9.0)

    Button
    {
        id:drawerData
        x:parent.width-width
        text:"打开侧边栏"
        z:1

        onClicked:
        {
            controlPanel.open()
        }
    }

    visible: true
    title: "仿shadertoy工具"
    color: "black"

    property var shaderList: []
    property string currentFile

    property var texturePaths: [
        ":qt/qml/MyRhi/assets/others/noiseInit.png",
        ":qt/qml/MyRhi/assets/others/picInit.jpg",
        ":qt/qml/MyRhi/assets/others/other.png"
    ]
    property var textureLabels: ["底噪(Ch1)", "背景(Ch2)", "其他(Ch3)"]
    property int currentTextureIndex: -1

    function changeShaderList(row,index)
    {
        shaderList[row].inputId=index
    }

    FileHelper
    {
        id:fileHelper
    }

    //rendering area
    RhiPingPongItem {
        id: renderer

        clip: true

        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.left: parent.left

        width: parent.height

        FrameAnimation {
            id: anim
            running: true
        }
        t: anim.elapsedTime

        MouseArea {
            anchors.fill: parent
            hoverEnabled: true

            onPositionChanged: {
                var limitX = Math.max(0, Math.min(width, mouseX))
                var limitY = Math.max(0, Math.min(height, mouseY))
                renderer.mousePos = Qt.point(limitX, limitY)
            }

            onPressed: renderer.isPressed = true
            onReleased: renderer.isPressed = false
        }
    }

    //editors
    ScrollView {
        id: scrollView
        // 1. 把原 TextArea 的布局属性移到这里，由 ScrollView 负责占位
        height: parent.height
        anchors.left: renderer.right
        anchors.right: parent.right

        // 2. 开启裁剪，防止文字滚出边界
        clip: true

        TextArea {
            id: shaderText
            // TextArea 内部不需要设高度和 anchors，它会自动撑开

            color: "white"
            font.family: "Consolas, 'Courier New', Monospace"
            font.pixelSize: 14

            // 3. 这里的 background 只需要给个颜色，ScrollView 会处理大小
            background: Rectangle {
                color: "black"
            }

            placeholderText: "// 在此输入 Fragment Shader 代码..."
            placeholderTextColor: "#666666"

            CodeHighlighter {
                id: highlighter
                document: shaderText.textDocument
            }

            Shortcut {
                sequence: StandardKey.Save
                onActivated: {
                    // 保持你原本的变量名 windwo
                    var success = fileHelper.saveFile(windwo.currentFile, shaderText.text)

                    if (success) {
                        console.log("✅ 保存成功:", windwo.currentFile)
                    } else {
                        console.log("❌ 保存失败")
                    }
                }
            }
        }
    }



    Drawer {
        id: controlPanel
        edge: Qt.RightEdge

        width: parent.width/5
        height: parent.height

        Rectangle
        {
            color: "black"
            anchors.fill: parent
        }

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 10
            spacing: 12


            Text {
                text: "Shader管线绑定"
                color: "white"
                font.bold: true
                font.pixelSize: 18
            }

            Switch {
                text: checked ? "🟢 激活" : "🔴 关闭"
                checked: renderer.running
                onCheckedChanged: renderer.running = checked

                palette.windowText: "white"
            }

            Button {
                text: "➕ 添加shader文件"
                Layout.fillWidth: true
                onClicked: shaderFileDialog.open()
            }

            CShaderView {

                id:shaderview

                shaderModel: shaderList
                Layout.fillHeight: true
                Layout.fillWidth: true


                onChangeInputId:(row,index)=>
                                {
                                    windwo.changeShaderList(row,index)
                                }

                onRemoveShader: (row)=>
                                {
                                    var temp = shaderList.slice(0);
                                    temp.splice(row, 1);
                                    shaderList = temp;
                                }

                onEditoShader: (path) => {

                                   var content = fileHelper.readFile(path)
                                   windwo.currentFile=path
                                   shaderText.text = content

                               }


            }

            Button {
                text: "清除"
                Layout.fillWidth: true
                onClicked: shaderList = []
            }

            Button {
                text: "▶️ 运行"
                Layout.fillWidth: true
                highlighted: true
                enabled: shaderList.length > 0
                onClicked: {
                    console.log("Starting Pipeline Build...")
                    renderer.running=false
                    var paths = []
                    var bindIds = []
                    for(var i = 0; i < shaderList.length; i++) {
                        paths.push(shaderList[i].path)
                        bindIds.push(shaderList[i].inputId)
                    }
                    console.log("Paths:", paths)
                    console.log("Bindings:", bindIds)
                    renderer.getFile(paths)
                    renderer.getArr(bindIds)
                    renderer.running=true
                }
            }

            Rectangle { Layout.fillWidth: true; height: 1; color: "gray" }
            Text {
                Layout.fillWidth: true
                text: "纹理图设置"
                color: "white"
                font.bold: true
                font.pixelSize: 18
            }
            Repeater {
                model: 3
                delegate: Column {
                    Layout.fillWidth: true
                    spacing: 4
                    Text {
                        text: textureLabels[index]
                        color: "#AAAAAA"
                        font.pixelSize: 12
                    }
                    Button {
                        width: parent.width
                        text: {
                            var path = texturePaths[index]
                            if (path.indexOf(":/") === 0) return "📦默认"
                            return "📂 " + path.split("/").pop()
                        }
                        onClicked: {
                            currentTextureIndex = index
                            textureFileDialog.open()
                        }
                    }
                }
            }
            Rectangle { width: parent.width; height: 1; color: "gray" }
            Text {
                text: "Time: " + renderer.t.toFixed(2)
                color: "white"
            }
        }
    }

    FileDialog {
        id: shaderFileDialog
        title: "Select Shader File"
        nameFilters: ["Shader files (*.frag *.vert *.glsl)", "All files (*)"]
        fileMode: FileDialog.OpenFile
        onAccepted: {
            var path = selectedFile.toString()
            if (Qt.platform.os === "windows" && path.indexOf("file:///") === 0) {
                path = path.slice(8)
            } else if (path.indexOf("file://") === 0) {
                path = path.slice(7)
            }
            var temp = shaderList
            temp.push({
                          path: path,
                          inputId: (temp.length > 0 ? temp.length - 1 : 0)
                      })
            shaderList = temp
        }
    }

    FileDialog {
        id: textureFileDialog
        title: "Select Texture Image"
        nameFilters: ["Images (*.png *.jpg *.jpeg *.bmp)", "All files (*)"]
        fileMode: FileDialog.OpenFile
        onAccepted: {
            var path = selectedFile.toString()
            if (Qt.platform.os === "windows" && path.indexOf("file:///") === 0) path = path.slice(8)
            var temp = texturePaths
            temp[currentTextureIndex] = path
            texturePaths = temp
            renderer.getTexUrl(texturePaths)
        }
    }
}
