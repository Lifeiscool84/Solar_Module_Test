# 🎯 **PROMPT ENGINEERING GUIDE**
## **How to Effectively Use AI Assistance with Your Professional Project Structure**

### **Your Role:** Senior Embedded Systems Engineer
### **AI Role:** Senior Development Assistant  
### **System:** Croc_Tracker_Dev_ref Professional Project Structure

---

## **🚀 FOUNDATION PROMPT TEMPLATE**

### **When Starting ANY Development Task, Use This Prompt:**

```
I am a senior embedded systems engineer working with a professional multi-sensor IoT project structure called "Croc_Tracker_Dev_ref". 

**Current Context:**
- I have a working 3-sensor INA228 power monitoring system  
- I want to extract RTC (RV8803) and GNSS (u-blox) functionality from my main.txt reference code
- I'm using the professional project structure with hardware/, firmware/, testing/, and documentation/ folders

**Project Structure:**
- Hardware Abstraction Layer: `hardware/sensors/[SENSOR_NAME]/`
- Firmware Layer: `firmware/reference_implementations/`
- Testing: `testing/unit_tests/` and `testing/hardware_validation/`
- Documentation: `documentation/[CATEGORY]/`
- Templates: `templates/[TYPE]/`

**My Current Task:** [DESCRIBE YOUR SPECIFIC TASK HERE]

**Expected Output:**
1. Follow the established project structure organization
2. Reference existing files (rtc_types.h, rv8803_driver.h, gnss_types.h)
3. Extract functionality from main.txt with proper line references
4. Create modular, reusable code components
5. Include testing recommendations
6. Suggest documentation updates

Please provide step-by-step guidance following professional embedded development practices.
```

---

## **📁 TASK-SPECIFIC PROMPT TEMPLATES**

### **🔧 FOR HARDWARE DRIVER DEVELOPMENT:**

```
**DRIVER DEVELOPMENT TASK**

I need to implement a hardware driver for [SENSOR_NAME] in my professional project structure.

**Context:**
- Project Structure: Croc_Tracker_Dev_ref/
- Target Location: `hardware/sensors/[SENSOR_NAME]/`
- Reference Code: Available in main.txt lines [START]-[END]
- Existing Pattern: Follow rv8803_driver.h/.cpp structure

**Requirements:**
1. Create [SENSOR_NAME]_types.h with data structures
2. Create [SENSOR_NAME]_driver.h with class interface  
3. Implement [SENSOR_NAME]_driver.cpp with extracted functions
4. Map these specific functions from main.txt:
   - [FUNCTION_1] → [NEW_METHOD_NAME]
   - [FUNCTION_2] → [NEW_METHOD_NAME]
   - [etc...]

**Expected Deliverables:**
- Clean header file with documented interface
- Implementation file with error handling
- Integration with existing project types
- References to original main.txt line numbers

**Quality Standards:**
- Professional coding practices
- Comprehensive error handling
- Clear documentation comments
- Modular, reusable design

Please implement following the established patterns in the project.
```

### **⚡ FOR FIRMWARE INTEGRATION:**

```
**FIRMWARE INTEGRATION TASK**

I need to create firmware-level coordination between multiple sensors.

**Integration Context:**
- Sensors: [LIST_OF_SENSORS] 
- Purpose: [DESCRIBE_FUNCTIONALITY]
- Location: `firmware/reference_implementations/`
- Dependencies: Hardware drivers in `hardware/sensors/`

**Coordination Requirements:**
1. [SPECIFIC_REQUIREMENT_1]
2. [SPECIFIC_REQUIREMENT_2]
3. [etc...]

**From main.txt, extract these coordination functions:**
- [FUNCTION_NAME] lines [START]-[END] → [NEW_SERVICE_METHOD]
- [FUNCTION_NAME] lines [START]-[END] → [NEW_SERVICE_METHOD]

**Expected Architecture:**
- Service class managing sensor coordination
- Clean interfaces between components  
- Error handling and fallback mechanisms
- Configuration management

Please create a firmware service following the established project patterns.
```

### **🧪 FOR TESTING DEVELOPMENT:**

```
**TESTING FRAMEWORK TASK**

I need to create comprehensive tests for [COMPONENT_NAME].

**Testing Context:**
- Component: [DESCRIBE_COMPONENT]
- Test Types Needed: Unit tests, Integration tests, Hardware validation
- Project Structure: `testing/unit_tests/`, `testing/integration_tests/`, `testing/hardware_validation/`

**Test Requirements:**
1. Unit Tests:
   - Test individual driver methods
   - Mock hardware interactions where appropriate
   - Validate error handling

2. Integration Tests:
   - Test sensor coordination
   - Validate data flow between components
   - Test failure scenarios

3. Hardware Validation:
   - Test with actual hardware
   - Validate sensor readings
   - Test I2C communication

**Reference Functionality:**
Extract test scenarios from main.txt functionality, especially:
- [DESCRIBE_KEY_FUNCTIONS_TO_TEST]

Please create a comprehensive testing framework following embedded best practices.
```

### **📚 FOR DOCUMENTATION CREATION:**

```
**DOCUMENTATION TASK**

I need to create technical documentation for [COMPONENT/INTEGRATION].

**Documentation Context:**
- Component: [DESCRIBE_WHAT_TO_DOCUMENT]
- Target Audience: [DEVELOPERS/USERS/BOTH]  
- Location: `documentation/[CATEGORY]/`
- Integration with existing docs: Link to PROJECT_STRUCTURE.md and DEVELOPMENT_WORKFLOW.md

**Documentation Requirements:**
1. **Technical Reference:**
   - API documentation
   - Configuration options
   - Integration steps

2. **User Guide:**
   - Setup instructions
   - Configuration examples
   - Troubleshooting guide

3. **Integration Guide:**
   - Step-by-step integration
   - Code examples
   - Common pitfalls and solutions

**Reference Material:**
Use functionality from main.txt as practical examples, especially:
- [SPECIFIC_FUNCTIONS_OR_WORKFLOWS]

Please create professional documentation following the established project documentation patterns.
```

---

## **💡 ADVANCED PROMPTING STRATEGIES**

### **📊 For Complex Analysis Tasks:**

```
**MULTI-STEP ANALYSIS TASK**

I need deep analysis and planning for [COMPLEX_TASK].

**Analysis Framework:**
Step 1: **Current State Analysis**
- Examine existing code in main.txt
- Identify key functionality blocks
- Map dependencies between components

Step 2: **Target Architecture Design**  
- Design modular component structure
- Define clean interfaces
- Plan testing strategy

Step 3: **Implementation Roadmap**
- Break down into development phases
- Identify risks and mitigation strategies
- Create timeline and milestones

Step 4: **Integration Planning**
- Define integration points
- Plan validation testing
- Document rollback strategies

**Specific Focus Areas:**
- [AREA_1]: [REQUIREMENTS]
- [AREA_2]: [REQUIREMENTS]  
- [etc...]

Please provide comprehensive analysis following senior engineering practices.
```

### **🔍 For Code Extraction and Modernization:**

```
**CODE MODERNIZATION TASK**

I need to extract and modernize functionality from main.txt.

**Modernization Context:**
- Source: main.txt lines [START]-[END]
- Target: Modern C++ with professional practices
- Integration: Fit into established project structure

**Modernization Requirements:**
1. **Code Quality:**
   - RAII patterns where appropriate
   - Clear error handling (avoid Arduino-style void functions)
   - Const correctness
   - Professional naming conventions

2. **Architecture:**
   - Separation of concerns
   - Dependency injection where beneficial
   - Configuration through structures/classes
   - Testable design

3. **Integration:**
   - Use existing project types and patterns
   - Maintain compatibility with existing drivers
   - Follow established documentation patterns

**Specific Functions to Modernize:**
- [FUNCTION_1]: [CURRENT_ISSUES_TO_ADDRESS]
- [FUNCTION_2]: [CURRENT_ISSUES_TO_ADDRESS]

Please modernize this code following senior embedded development practices.
```

---

## **🎯 RESULT QUALITY VALIDATION PROMPTS**

### **After Receiving AI Output, Use These Validation Prompts:**

```
**CODE REVIEW VALIDATION**

Please review the code you just provided and validate:

1. **Project Structure Compliance:**
   - Files placed in correct directories?
   - Following established naming conventions?
   - Proper include paths for project structure?

2. **Code Quality:**
   - Error handling comprehensive?
   - Documentation comments clear and complete?
   - Interface design clean and intuitive?

3. **Integration Readiness:**
   - Compatible with existing project patterns?
   - Dependencies clearly defined?
   - Testing considerations addressed?

4. **Professional Standards:**
   - Follows embedded development best practices?
   - Scalable for future sensor additions?
   - Maintainable code structure?

If you identify any issues, please provide corrections.
```

---

## **📋 PROMPT CHECKLISTS**

### **Before Submitting Any Development Prompt:**

- [ ] Clearly stated your role (senior embedded engineer)
- [ ] Mentioned the project structure (Croc_Tracker_Dev_ref)
- [ ] Specified target folder/location
- [ ] Referenced existing patterns to follow
- [ ] Included specific line numbers from main.txt if applicable
- [ ] Defined quality expectations
- [ ] Requested specific deliverables

### **For Complex Multi-Step Tasks:**

- [ ] Broken task into logical phases
- [ ] Specified dependencies between phases
- [ ] Requested step-by-step guidance
- [ ] Included testing considerations
- [ ] Mentioned documentation requirements
- [ ] Asked for professional development practices

---

## **🚀 EXAMPLE: COMPLETE RTC DRIVER DEVELOPMENT PROMPT**

```
I am a senior embedded systems engineer working with a professional multi-sensor IoT project structure called "Croc_Tracker_Dev_ref".

**Current Context:**
- I have established project structure with hardware/, firmware/, testing/, documentation/ folders
- I have created rtc_types.h and rv8803_driver.h interface files
- I need to implement rv8803_driver.cpp by extracting functionality from main.txt

**Specific Task:**
Implement rv8803_driver.cpp following the interface defined in rv8803_driver.h

**Code Extraction Requirements:**
From main.txt, extract and modernize these functions:
- `initRTC()` (lines ~1055-1120) → `RV8803_Driver::initialize()`
- `setTimeZone()` (lines ~1180-1200) → `RV8803_Driver::setTimezone()`
- `updateRTCTimestamp()` (lines ~1220-1280) → `RV8803_Driver::getTimestamp()`
- `adjustTimeSeconds()` (lines ~1350-1400) → `RV8803_Driver::applyTimeAdjustment()`

**Quality Requirements:**
- Use existing RTCStatus enum for error handling
- Integrate with RTCTimestamp and RTCConfig structures  
- Include comprehensive error checking
- Add detailed documentation comments
- Follow established patterns from project structure

**Integration Requirements:**
- File location: `hardware/sensors/rtc/rv8803_driver.cpp`
- Include proper headers from project structure
- Use Wire library for I2C communication
- Maintain compatibility with SparkFun RV8803 library

**Expected Deliverables:**
1. Complete rv8803_driver.cpp implementation
2. Constructor implementations
3. All public method implementations from header
4. Private helper method implementations
5. Comprehensive error handling

Please implement this driver following professional embedded development practices and maintaining traceability to the original main.txt functions.
```

---

## **📞 FINAL GUIDANCE**

### **As Your Senior Prompt Engineer Mentor:**

1. **Always Start with Context** - AI needs to understand your professional project structure
2. **Be Specific About Deliverables** - Don't just ask for "help", define exactly what you need
3. **Reference Existing Patterns** - Point to files that show the style/structure to follow
4. **Include Quality Criteria** - Define your professional standards upfront
5. **Validate Results** - Always review AI output against your requirements

### **Remember:**
- This project structure is your framework for scalable development
- Each sensor addition follows the same pattern
- Documentation and testing are as important as implementation
- Professional practices today save debugging time tomorrow

**🎓 Your success with this system depends on clear, specific prompts that leverage the professional structure we've built together.**

---
**Guide Version:** 1.0  
**Author:** Senior Prompt Engineer at Anthropic  
**Last Updated:** January 2025 