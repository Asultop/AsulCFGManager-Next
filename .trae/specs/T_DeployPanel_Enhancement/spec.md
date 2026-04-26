# T_DeployPanel 完善规范

## Why
当前T_DeployPanel构造函数存在以下问题：
1. MustField处理不完整（只有声明没有实现）
2. 缺少TextField、LineField、OptionField类型的处理
3. 没有处理enabledExpression可见性关系

## What Changes
- 完善MustFieldNode的处理逻辑
- 添加TextFieldNode、LineFieldNode、OptionFieldNode的处理
- 实现基于enabledExpression的Visible关系处理
- 所有Form内容在一个CFGSettingPage中显示
- Group使用ElaDrawer展示

## Impact
- Affected code: `T_DeployPanel.cpp` 构造函数（第12-135行）
- 可添加私有辅助函数

## ADDED Requirements
### Requirement: KeyBindNode 处理
系统应当创建ElaKeyBinder和ElaLineEdit两个控件，支持双向同步，处理特殊键值转换（Up→uparrow等）

#### Scenario: KeyBind字段显示
- **WHEN** 渲染KeyBindNode类型字段
- **THEN** 显示keyBinder和keyBinderLine，值同步到KeyBindField->bind

### Requirement: MustFieldNode 处理
系统应当创建只读ElaLineEdit显示MustField的值

#### Scenario: MustField字段显示
- **WHEN** 渲染MustFieldNode类型字段
- **THEN** 显示只读的bind值

### Requirement: TextFieldNode 处理
系统应当直接显示文本内容，不创建额外控件

#### Scenario: TextField字段显示
- **WHEN** 渲染TextFieldNode类型字段
- **THEN** 仅显示Description和SubDescription

### Requirement: LineFieldNode 处理
系统应当为每个Arg创建Label+LineEdit行，并支持表达式输入

#### Scenario: LineField字段显示
- **WHEN** 渲染LineFieldNode类型字段
- **THEN** 显示args列表和表达式输入

### Requirement: OptionFieldNode 处理
系统应当创建ComboBox显示所有选项，支持选择

#### Scenario: OptionField字段显示
- **WHEN** 渲染OptionFieldNode类型字段
- **THEN** 显示ComboBox，当前选中为selected值

### Requirement: Visible关系处理
系统应当根据enabledExpression评估结果控制字段可见性

#### Scenario: 字段可见性更新
- **WHEN** 字段值发生变化
- **THEN** 重新评估所有字段的enabledExpression并更新可见性

## MODIFIED Requirements
### Requirement: 主循环逻辑
在已有循环基础上添加fieldWidgetMap和fieldAreaMap存储widget引用，设置初始可见性并连接变化信号
