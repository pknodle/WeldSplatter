#include "plugin.hpp"

#include <app/LedDisplay.hpp>
#include <sstream>

struct WeldSplatter_AcornTable : Module {
  enum ParamIds {
		 // The first 144 parameters are for the grid of buttons
		 // And recall that they are zero indexed
		 TEACH_MODE_PARAM = 144,
		 TEACH_NOTE_PARAM,
                 ALLOW_REPITITION_PARAM,
                 USE_EXT_PARAM,
                 NUM_PARAMS
  };
  enum InputIds
    {
     TEACH_NOTE_INPUT,
     TEACH_TRIGGER_INPUT,
     ROW_INPUT,
     COL_INPUT,
     NUM_INPUTS
    };
  enum OutputIds
    {
     NOTE_OUTPUT,
     TRIG_OUTPUT,
     GATE_OUTPUT,
     NUM_OUTPUTS
    };
  
  enum LightIds
    {
     LIGHT_0 = 0,
     LIGHT_1,
     LIGHT_2,
     LIGHT_3,
     LIGHT_4,
     LIGHT_5,
     LIGHT_6,
     LIGHT_7,
     LIGHT_8,
     LIGHT_9,
     LIGHT_10,
     LIGHT_11,
     NUM_LIGHTS
    };

  Label* teach_mode_indicators;

  float note_voltages[12]{ 1.00, 1.08, 1.17, 1.25, 1.33, 1.42, 1.5, 1.58, 1.67, 1.75, 1.83, 1.92};
  
  int volt_to_note(float v){
    for(int i = 0; i < 12; i++){
      if(v < (note_voltages[i] + 0.01)){
	return i;
      }
    }
    return -1;
  }
  
  WeldSplatter_AcornTable() {
    config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
    

    for(int i = 0; i < 12; i++){
      for(int j = 0; i < 12; i++){
	int index = i * 12 + j;
	configParam(index, 0.0, 1.0, 0.0);
      }
    }
    configParam(TEACH_MODE_PARAM, 0.0, 1.0, 0.0, "Teach Mode");
    configParam(ALLOW_REPITITION_PARAM, 0.0, 1.0, 0.0, "Allow Rep");
    configParam(USE_EXT_PARAM, 0.0, 1.0, 0.0, "Ext Mode");

    
    for(int i = 0; i < 12; i++){
      taught_12_tone_row[i] = i;
    }


    teach_mode_indicators = nullptr;

    
    generate_matrix();
  }


  void generate_matrix(){
    // First, copy the taught row to the first row of the matrix
    int i, j;
    for(i = 0; i < 12; i++){
      note_matrix[0][i] = taught_12_tone_row[i];
    }

    // Now generate the inversion in the first column
    for(j = 0; j < 12; j++){
      note_matrix[j][0] = (12 - note_matrix[0][j]) % 12;
    }

    // Now generate the remaining rows
    for(i = 1; i < 12; i++){
      for(j = 1; j < 12; j++){
        note_matrix[i][j] = (note_matrix[i][0] + note_matrix[0][j]) % 12;
      }
    }
  }
  
  int note_matrix[12][12];

  int taught_12_tone_row[12];
  int teach_index = 0;
  bool taught_notes = false;

  int last_i_cap = -1;
  int last_j_cap = -1;

  int last_ext_row = -1;
  int last_ext_col = -1;
  
  int debug_count = 0;
  
  dsp::PulseGenerator trig_gen;
  dsp::SchmittTrigger teach_trigger;

  bool teach_gate_mask = 1;

  bool is_note_in_row(int note){
    for(int i = 0; i < 12; i++){
      if(taught_12_tone_row[i] == note){
        return true;
      }
    }
    return false;
  }
  
  void process_teach_mode(const ProcessArgs& args) {
    if(!taught_notes){
      // Reset to the begining of the row
      teach_index = 0;
      taught_notes = true;
      for(int i = 0; i < 12; i++){
        taught_12_tone_row[i] = -1;
        lights[i].setBrightness(0.0);
      }
      lights[0].setBrightness(1.0);
      teach_mode_indicators->text = std::string{"- - - - - - - - - - - -"};
    }

    
    float note_input__volts = inputs[TEACH_NOTE_INPUT].getVoltage();
    int note_input__number = volt_to_note(note_input__volts);
    
    // We only want to use the range of the top of the keyboard (computer keyboard)
    // I'll probably change this later.
    bool trigger = teach_trigger.process(inputs[TEACH_TRIGGER_INPUT].getVoltage());
    bool allow_repitition = params[ALLOW_REPITITION_PARAM].getValue() > 0.5;

    bool note_in_row = is_note_in_row(note_input__number);


    // If you don't allow repititoin in your row (ie, strict mode)
    // then don't play a note that is entered by the user. 
    if(trigger && note_input__number && !allow_repitition){
      teach_gate_mask = false;
    }
    
    if(trigger && (note_input__number >= 0) && (allow_repitition || !note_in_row)){
      taught_notes = true;
      DEBUG("Trigger");
      taught_12_tone_row[teach_index] = note_input__number;

      teach_gate_mask = true;
      
      // Move the light to the next note in the row
      lights[teach_index].setBrightness(0.0);
      if(teach_index < 11){
        lights[teach_index+1].setBrightness(1.0);
      }else{
        // We've taught all the notes
        for(int i = 0; i < 12; i++){
          lights[i].setBrightness(1.0);
          params[TEACH_MODE_PARAM].setValue(0.0);
        }
      }

      // Now update the note indicators
      std::stringstream ss;
      for(int i = 0; i < 12; i++){
        if(taught_12_tone_row[i] != -1){
          ss << " " << taught_12_tone_row[i];
        }else{
          ss << " -";
        }
      }
      std::string display = ss.str();

      if(teach_mode_indicators != nullptr){
        DEBUG("Indicate: %s %p", display.c_str(), teach_mode_indicators);
        teach_mode_indicators->text = display;

      }

      teach_index = (teach_index + 1) % 12;
    }
    // We want to act like a normal keyboard when in teaching mode.
    // This will let the user hear the 12-tone row as they teach it.
    // (Give users feedback and all that...)
    outputs[NOTE_OUTPUT].setVoltage(inputs[TEACH_NOTE_INPUT].getVoltage());
    if((note_input__number >= 0) && teach_gate_mask ) {
      outputs[GATE_OUTPUT].setVoltage(inputs[TEACH_TRIGGER_INPUT].getVoltage());
    }else{
      outputs[GATE_OUTPUT].setVoltage(0.0);
    }
  }


  void process_pad_mode(const ProcessArgs& args){
    int i = 0;
    int j = 0;
    int i_cap = -1;
    int j_cap = -1;
    
    // I'm only doing this monophonic right now.
    int index = 0;
    for(i = 0; i < 12; i++){
      for(j = 0; j < 12; j++){
	index = i * 12 + j;
	if(params[index].getValue() > 0.5){
          i_cap = i;
          j_cap = j;
	}
      }
    }


    if((last_i_cap != i_cap) || (last_j_cap != j_cap)){
      if((i_cap == -1) && (j_cap == -1)){
        // The user just released the buton
        // (Falling edge)
        outputs[GATE_OUTPUT].setVoltage(0.0f);
      }else{
        // The user just hit a new button (ie new note)
        // (Rising edge)
        int output_note__number = note_matrix[i_cap][j_cap];
        float output_note__vPo = note_voltages[output_note__number];
        outputs[NOTE_OUTPUT].setVoltage(output_note__vPo);
        outputs[GATE_OUTPUT].setVoltage(10.0f);
        trig_gen.trigger(1e-3f);
      }
      
    }
    last_i_cap = i_cap;
    last_j_cap = j_cap;

    if(trig_gen.process(1 / args.sampleRate)){
      outputs[TRIG_OUTPUT].setVoltage(10.0f);
    }else{
      outputs[TRIG_OUTPUT].setVoltage(0.0f);
    }

  }

  void process_ext_mode(const ProcessArgs& args){
    float row__volts = inputs[ROW_INPUT].getVoltage();
    float col__volts = inputs[COL_INPUT].getVoltage();

    float step_size = 11.999f/10.0f;

    int row = (int) ( row__volts * step_size);
    int col = (int) ( col__volts * step_size);

    bool row_in_range = (row >= 0) && (row < 12);
    bool col_in_range = (col >= 0) && (row < 12);

    if(!row_in_range || !col_in_range){
      return;
    }
    
    
    if((row != last_ext_row) || (col != last_ext_col)){
      // We moved to a new note
      trig_gen.trigger(1e-3f);
      int output_note__number = note_matrix[row][col];
      float output_note__volts = note_voltages[output_note__number];
      outputs[NOTE_OUTPUT].setVoltage(output_note__volts);
      
      int index = last_ext_row * 12 + last_ext_col;
      params[index].setValue(0.0);
      index = row * 12 + col;

      // Check against TEACH_MODE_PARAM to guard agaisnt bugs.
      // All the params below that are in the grid of buttons
      if(index < TEACH_MODE_PARAM){
        params[index].setValue(1.0);
      }

      DEBUG("EXT R: %d C: %d", row, col);

      last_ext_row = row;
      last_ext_col = col;

    }else{

    }

    if(trig_gen.process(1 / args.sampleRate)){
      outputs[TRIG_OUTPUT].setVoltage(10.0f);
    }else{
      outputs[TRIG_OUTPUT].setVoltage(0.0f);
    }


  }


  void process(const ProcessArgs& args) override {
    

    if((++debug_count % 44000) == 0){
      debug_count = 0;
      DEBUG("Row: %02d %02d %02d %02d %02d %02d %02d %02d %02d %02d %02d %02d",
            taught_12_tone_row[0], taught_12_tone_row[1], taught_12_tone_row[2], taught_12_tone_row[3],
            taught_12_tone_row[4], taught_12_tone_row[5], taught_12_tone_row[6], taught_12_tone_row[7],
            taught_12_tone_row[8], taught_12_tone_row[9], taught_12_tone_row[10], taught_12_tone_row[11]);
    }
    

    
    if(params[TEACH_MODE_PARAM].getValue() > 0.5){
      process_teach_mode(args);
    }  else if(taught_notes) {
      // This is a trigger when leaving teaching mode to recompute the 12-tone matrix
      taught_notes = false;
      generate_matrix();
      for(int r = 0; r < 12; r++){
        DEBUG("%2d | %2d %2d %2d %2d %2d %2d %2d %2d %2d %2d %2d %2d", r,
              note_matrix[r][0], note_matrix[r][1], note_matrix[r][2] , note_matrix[r][3],
              note_matrix[r][4], note_matrix[r][5], note_matrix[r][6] , note_matrix[r][7],
              note_matrix[r][8], note_matrix[r][9], note_matrix[r][10], note_matrix[r][11]);
      }
    }else{
      // Playback mode
      if(params[USE_EXT_PARAM].getValue() < 0.5){
        process_pad_mode(args);
      }else{
        process_ext_mode(args);
      }
    }
   
  }


  json_t* dataToJson () override{
    auto taught_row_json = json_array();

    for(int i = 0; i < 12; i++){
      json_array_append_new(taught_row_json, json_integer(taught_12_tone_row[i]));
    }
    
    return taught_row_json;
  }

  void dataFromJson (json_t *root) override {
    for(int i = 0; i < 12; i++){
      taught_12_tone_row[i] =  json_integer_value(json_array_get(root, i));
    }
    generate_matrix();
  }
  

}; // End class/struct

struct SmallButton : SVGSwitch {
    SmallButton() {
      momentary = true;
      addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/SmallButtonDown.svg")));
      addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/SmallButtonUp.svg")));
      
    }
  };

struct ToggleButton : SVGSwitch {
    ToggleButton() {
      momentary = false;
      addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/SmallButtonDown.svg")));
      addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/SmallButtonUp.svg")));
      
    }
  };



struct WeldSplatter_AcornTableWidget : ModuleWidget {
    WeldSplatter_AcornTableWidget(WeldSplatter_AcornTable* module) {
      setModule(module);
      setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/acorn-table.svg")));

      addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
      addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
      addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
      addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));


      auto w = createWidget<Label>(mm2px(Vec(10, 75)));
      w->text = std::string{"- - - - - - - - - - - -"};
      w->color =  nvgRGB(0,0,0);
      //w->setSize(mm2px(Vec(65, 10)));
      addChild(w);

      if(module != nullptr){
        module->teach_mode_indicators = w;
      }


      
      addInput(createInputCentered<PJ301MPort>(mm2px(Vec(30, 120)), module, WeldSplatter_AcornTable::TEACH_NOTE_INPUT));
      addInput(createInputCentered<PJ301MPort>(mm2px(Vec(40, 120)), module, WeldSplatter_AcornTable::TEACH_TRIGGER_INPUT));

      addInput(createInputCentered<PJ301MPort>(mm2px(Vec(50, 120)), module, WeldSplatter_AcornTable::ROW_INPUT));
      addInput(createInputCentered<PJ301MPort>(mm2px(Vec(60, 120)), module, WeldSplatter_AcornTable::COL_INPUT));

      for(int i = 0; i < 12; i++){
        addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(10 + i*5.0, 70)), module, i));
      }
      
      addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(40, 50 )), module, WeldSplatter_AcornTable::NOTE_OUTPUT));
      addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(50, 50 )), module, WeldSplatter_AcornTable::TRIG_OUTPUT));
      addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(60, 50 )), module, WeldSplatter_AcornTable::GATE_OUTPUT));

      addParam(createParam<ToggleButton>(mm2px(Vec(30.0, 15.0)), module, WeldSplatter_AcornTable::TEACH_MODE_PARAM));
      addParam(createParam<ToggleButton>(mm2px(Vec(40.0, 15.0)), module, WeldSplatter_AcornTable::ALLOW_REPITITION_PARAM));
      addParam(createParam<ToggleButton>(mm2px(Vec(50.0, 15.0)), module, WeldSplatter_AcornTable::USE_EXT_PARAM)); 
      
      
      Vec firstButton(75.0, 5.0);
      Vec col_offset(10.0, 0);
      Vec row_offset(0.0, 10.0);
            
      for(int n = 0; n < 12; n++){
	for(int m = 0; m < 12; m++) {
	  Vec row = row_offset.mult(n);
	  Vec col = col_offset.mult(m);
	  Vec pos = firstButton.plus(row.plus(col));
	  
	  int index = n * 12 + m;
	  addParam(createParam<SmallButton>(mm2px(pos), module, index)); 
	}
      }
      
    } // Closes constructor

  void draw(const DrawArgs& args) override {
    ModuleWidget::draw(args);
  }
};


Model* modelWeldSplatter_AcornTable = createModel<WeldSplatter_AcornTable, WeldSplatter_AcornTableWidget>("WeldSplatter-AcornTable");
