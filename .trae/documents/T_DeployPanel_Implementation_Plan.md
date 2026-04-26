# T\_DeployPanel.cpp 完善计划

## 目标

完善 `T_DeployPanel.cpp` 的构造函数，实现：

1. 将Form所有内容在一个CFGSettingPage显示
2. Group使用Drawer
3. 正确处理Visible关系（enabledExpression）

## 当前代码分析

### 问题

1. **MustField处理不完整** - 第125-127行只有声明，没有实现
2. **缺少其他Field类型处理** - 没有处理 TextField、LineField、OptionField
3. **没有处理Visible关系** - 没有使用 `doc->evaluateFieldEnabled()` 来控制字段可见性

### AFormParser结构

```
Document
  └── forms (QVector<FormNode>)
        └── groups (QVector<GroupNode>)
              └── fields (QVector<FieldNode>)
                    ├── KeyBindNode (command, bind)
                    ├── MustFieldNode (command, bind)
                    ├── TextFieldNode (text, command)
                    ├── LineFieldNode (args, expression)
                    └── OptionFieldNode (options, selected)
```

### FieldNode通用属性

* `id` - 字段标识符

* `enabledExpression` - 可见性表达式（Lua）

* `description` - 主描述

* `subDescription` - 副描述

## 实现方案

### 1. 添加辅助函数

#### `createFieldWidget(FieldNode::Ptr field, QWidget* parent) -> QWidget*`

根据field类型创建对应的输入控件：

* **KeyBindNode** → ElaKeyBinder + ElaLineEdit

* **MustFieldNode** → 只读ElaLineEdit

* **TextFieldNode** → ElaLineEdit

* **LineFieldNode** → 多个ElaLineEdit（对应args）+ 表达式输入

* **OptionFieldNode** → QComboBox

* **其他** → 返回nullptr

#### `setupFieldConnections(FieldNode::Ptr field, QWidget* widget, Document::Ptr doc)`

当字段值改变时，触发所有字段的可见性重新评估

#### `updateAllFieldsVisibility(Document::Ptr doc, const QMap<QString, QWidget*>& fieldWidgets)`

遍历所有字段控件，根据 `doc->evaluateFieldEnabled(fieldId)` 设置可见性

### 2. 修改主循环逻辑

```cpp
// 存储field的widget引用，用于可见性更新
QMap<QString, QWidget*> fieldWidgetMap;
QMap<QString, ElaScrollPageArea*> fieldAreaMap;

for (const auto &form : doc->forms) {
    //创建一个 字体稍大的ElaText直接添加到 centerVLayout （按顺序）
    for (const auto &group : form->groups) {
        // 创建Drawer (使用group->title作为标题)
        ElaDrawerArea *drawerArea = new ElaDrawerArea(CFGSettingPage);
        // ...

        for (const auto &field : group->fields) {
            QWidget *configWidget = createFieldWidget(field, CFGSettingPage);

            // 使用 GenerateArea 创建带标题的控件区
            ElaScrollPageArea *area = gFunc->GenerateArea(CFGSettingPage,
                new ElaText(field->description, CFGSettingPage),
                new ElaText(field->subDescription, CFGSettingPage),
                configWidget, false);

            // 存储映射
            fieldWidgetMap[field->id] = configWidget;
            fieldAreaMap[field->id] = area;

            drawerArea->addDrawer(area);

            // 设置初始可见性
            bool enabled = doc->evaluateFieldEnabled(field->id);
            configWidget->setVisible(enabled);
            area->setVisible(enabled);

            // 连接字段值变化信号以更新可见性
            setupFieldConnections(field, configWidget, doc);
        }
    }
}

// 初始可见性更新
updateAllFieldsVisibility(doc, fieldWidgetMap);
```

### 3. 关键实现细节

#### KeyBindNode 处理

* 创建ElaKeyBinder用于选择按键

* 创建ElaLineEdit用于直接输入

* 双向同步：keyBinder ↔ keyBinderLine ↔ KeyBindField->bind

* 特殊键值转换：Up→uparrow, Down→downarrow等

#### MustFieldNode 处理

* 创建只读ElaLineEdit

* 创建只读ElaKeyBinder

* 显示bind值

#### TextFieldNode 处理

* 不创建任何额外控件，只展示默认Description和SubDescription

#### LineFieldNode 处理

* 不使用GenerateArea而是使用ElaDrawerArea

- 为每个ArgNode创建一行：Label + ElaLineEdit

- 绑定 value ↔ ArgNode->value

- 表达式字段使用 ElaLineEdit

#### OptionFieldNode 处理

* 创建ElaComboBox

* 添加所有options，data设置为option->id

* 绑定 selected ↔ OptionFieldNode->selected

#### Visible关系处理

* 字段值变化时，调用 `updateAllFieldsVisibility()`

* 使用 `doc->evaluateFieldEnabled(fieldId)` 获取当前可见性状态

* 通过 `QWidget::setVisible()` 控制

### 4. 文件修改范围

仅修改 `T_DeployPanel.cpp` 的构造函数（12-135行），可以添加辅助函数在类内部或作为私有成员函数。

### 5. 头文件依赖

确认已包含的头文件：

* `T_DeployPanel.h`

* `GlobalFunc.h` (GenerateArea)

* `ElaDrawerArea`

* `ElaKeyBinder`

* `ElaLineEdit`

可能需要添加：

* `ElaComboBox` (用于OptionField)

* &#x20;`ElaText` (用于LineField的args标签)

