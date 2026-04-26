# Tasks

## 1. 创建辅助函数 `createFieldWidget`
实现根据field类型创建对应控件的函数

### SubTask 1.1: 实现KeyBindNode处理
创建ElaKeyBinder + ElaLineEdit，处理键值转换

### SubTask 1.2: 实现MustFieldNode处理
创建只读ElaLineEdit显示bind值

### SubTask 1.3: 实现TextFieldNode处理
返回nullptr（只显示描述不创建控件）

### SubTask 1.4: 实现LineFieldNode处理
为每个Arg创建Label+LineEdit行

### SubTask 1.5: 实现OptionFieldNode处理
创建QComboBox添加所有选项

## 2. 创建辅助函数 `setupFieldConnections`
当字段值改变时触发可见性重新评估

## 3. 创建辅助函数 `updateAllFieldsVisibility`
遍历所有字段控件，根据evaluateFieldEnabled设置可见性

## 4. 修改主循环逻辑
- 添加fieldWidgetMap和fieldAreaMap
- 设置初始可见性
- 连接变化信号
- 初始调用updateAllFieldsVisibility

## 5. 验证实现
编译检查并确认所有field类型都有处理

- [x] Task 1: 创建辅助函数createFieldWidget - KeyBind/Must/Text/Line/Option处理
- [x] Task 2: 创建辅助函数setupFieldConnections
- [x] Task 3: 创建辅助函数updateAllFieldsVisibility
- [x] Task 4: 修改主循环逻辑 - 添加widget映射和可见性处理
- [ ] Task 5: 验证实现 - 编译检查
