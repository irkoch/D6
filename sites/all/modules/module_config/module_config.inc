<?php

include_once 'module_config_model.inc';

/* Contain a pricipal functions to process
 * configuration files and variables data
 * @author Chokri Tliche
 */

class Module_config
{
	var $config_data = array();	
	var $module_list = array();
	var $module_config_list = array();
	var $data = array();
        
	/*
	 * main function to load module config data
	 * and clear data for uninstalled resources
	 * @return array $config_data populated with simple XML objects
	 */

	function module_config_load_config_data()
	{
            $mc_model = new Module_config_model();
	    $this->module_list = module_list();
	    $mc_model->mc_modules_list();

	    foreach($this->module_list as $module)
	    {
	        $module_config_file = drupal_get_path('module', $module) . '/config.xml';

	        if(file_exists($module_config_file))
	        {
	            $conf_content = simplexml_load_file($module_config_file);            
	            if(isset($conf_content->$module))
	            {
	                $this->config_data[$module] = $conf_content;                        
                        $mc_model->mc_module_set_status($module);

	                unset($mc_model->modules_list[$module]);
	            }
	        }
	    }

	    if(!empty($mc_model->modules_list))
	    {// clean unuseful module variables
	      $mc_model->module_config_clean_data();
	    }
	}
	
	/*
	 * Process config data XML and save
	 * new resources loaded data
	 * @param object $config_data Simple XML
	 */

	function module_config_process_config_data()
	{   
	    foreach($this->config_data as $module => $conf)
	    {        
	        $this->module_config_xml_get_child($conf, $this->data[$module]);
                $this->module_config_config_save($module);
	        $this->config_data[$module] = $this->data[$module];
	    }
	}
	
	/*
	 * convert XML data to array
	 * @param object $conf XML data
	 * @param array $data converted data
	 */

	function module_config_xml_get_child($conf, &$data)
	{    
	    foreach((array) $conf as $key => $value)
	    {
	        if(!is_object($value))
	        {
	            $data[$key] = $value;
	        }
	        else
	        {            
	            if(count((array) $value) == 1 and is_array(current($value)))
	            {                       
	                $this->module_config_xml_get_child(current($value), $data[$key]);
	            }
	            else{              
	                $this->module_config_xml_get_child($value, $data[$key]);
	            }
	        }
	    }   
	}

         /*
         * process loaded data to database
         * @param associative array $data
         * @param string $module resource name
         */

        function module_config_config_save($module)
        {
            $mc_model = new Module_config_model();
            $module_config = $mc_model->module_config_get_module_config($module);
            
            if(empty($module_config))
            {
                $module_config = $mc_model->mc_insert_module_config($module,
                                    $this->data[$module]['@attributes']['type'],
                                    $this->data[$module]['active']);
            }


            foreach($this->data[$module]['groups'] as $gkey => $group)
            {
                if(!is_array($group['fields']))
                {
                  drupal_set_message('XML Error');
                  continue;
                }

                foreach ($group['fields'] as $fkey => $field)
                {
                    $field_name = $field['@attributes']['name'];
                    if($field_name and !isset($module_config['variables'][$field_name]))
                    {
                        $values = $mc_model->mc_insert_module_variable($module_config['id'],
                                    $field['@attributes']['name'],
                                    $field['default_value']);

                        if($values){
                            $module_config['variables'][$values['scid'].'#'.$values['name']] = $values;

                            $data_field_value = &$data['groups'][$gkey]['fields'][$fkey];
                            unset($data_field_value['@attributes']);

                            $data_field_value = array_merge($data_field_value,
                                 array('@attributes' =>
                                     array('name' => $values['scid'].'#'.$values['name'])));
                        }

                    }
                     else {
                        $reset_default = FALSE;
                        if(isset($_SESSION['module_config'])
                            and $_SESSION['module_config'] == 'reset')
                        {
                            $reset_default = TRUE;
                        }
                        $fvalue = $module_config['variables'][$field_name];
                        $data_field_value = &$this->data[$module]['groups'][$gkey]['fields'][$fkey];

                        unset($data_field_value['@attributes']);

                        $data_field_value = array_merge($data_field_value,
                             array('@attributes' =>
                                array('name' => $fvalue['scid'].'#'.$fvalue['name'])));

                        if(!$reset_default)
                        {
                            if('file' == $data_field_value['type'])
                            {
                                $file = $fvalue['value'];
                                if(isset($file->filepath))
                                  $data_field_value['suffix'] = theme('image', $file->filepath);
                            }
                            else
                            {
                                $data_field_value = array_merge($data_field_value,
                                     array('default_value' => $fvalue['value']));
                            }
                        }

                        $module_config['variables']
                            [$fvalue['scid'].'#'.$fvalue['name']] = $fvalue;
                        unset($module_config['variables'][$field_name]);
                    }
                }
            } //end foreach
            unset($_SESSION['module_config']);
        }
	
	
	function get_config_data(){
            return $this->config_data;
	}
		
} // class end


/*
 * save post group fields
 * @param array $fields
 * @param array $form_fields
 */

function _module_config_variable_save($fields, &$form_fields)
{    
    foreach($fields as $fkey => $field)
    {      
        if(!empty ($field))
        {
            list ($k, $v) = array(current(array_keys($field)), current($field));
            list($scid, $name) = split('#', $k);
            
            // get field type to process file field
            $file_type = $form_fields[$fkey][$k]['#type'];
            
            if($file_type == 'file' and isset ($_FILES['files']))
            {              
                foreach(array_keys($_FILES['files']['name']) as $fname)
                {                  
                  if($_FILES['files']['tmp_name']
                          and (isset($_FILES['files']['error'][$fname])
                          and !$_FILES['files']['error'][$fname]))
                  {                      
                      $validators = array();
                      $file = file_save_upload($fname, $validators, file_directory_path());
                      file_set_status($file, FILE_STATUS_PERMANENT);
                      $v = $file;                      
                  }
                  elseif($_FILES['files']['tmp_name'][$fname])
                  {
                      drupal_set_message(t('File upload Error'), 'Module Config');
                  }
                }
            }
            
            if($scid and $name) //and !empty ($v)
            {   // processing fiel field resource              
                $query = 'UPDATE {module_config_variable} SET value = \'%s\'
                    WHERE scid = %d AND name = \'%s\' ';
                
                if(!db_query($query, serialize($v), $scid, $name))
                {
                  drupal_set_message('Error saving field...');
                }
            }            
        }
    }
}

/*
 * construct fieldsets from module config groups
 * @param array $groups list of configuration group
 * @return array $groups constructed from fieldset
 */

function _module_config_groups($groups)
{  
    foreach($groups as $gkey => $group)
    {        
        $groups[$gkey] = array(
              '#type' => 'fieldset',
              '#title' => $group['title'],
              '#weight' => $group['weight'],
              '#collapsible' => ($group['collapsible'] == 'false')? FALSE : TRUE,
              '#collapsed' => ($group['collapsed'] == 'false')? FALSE : TRUE,
              '#tree' => TRUE,
              'fields' => array()
        );      
        
        if(!is_array($group['fields']))
        {
          continue;
        }

        foreach($group['fields'] as $fkey => $field)
        {                    
          $groups[$gkey]['fields'][$fkey] = _module_config_field($field);
        }
       
        usort( $groups[$gkey]['fields'] , 'sort_field_by_weight' );        
    }    
    return $groups;
}

/*
 * sort multi dimentional fields array by weight
 * @param array $a
 * @param array $b
 * @return bool comparison result
 */

function sort_field_by_weight($a, $b)
{
  $a = current($a);
  $b = current($b);
  return (intval($a['#weight']) > intval($b['#weight']));
}

/*
 * construct drupal form field from module config field
 * @param array $field list of field options
 * @return array $field constructed field
 */

function _module_config_field($field)
{    
    $fname = $field['@attributes']['name'];
    unset($field['@attributes']);
    foreach($field as $key => $value)
    {
        if('options' === $key){
            $x = 0;
            foreach($value as $okey => $option)
            {
                if(is_array($option))
                {
                    if(++$x == 1)
                      $value[''] = '- '.t('Select').' -';
                    
                    $value[$option['@attributes']['value']] = $option['label'];
                    unset($value[$okey]);
                }
            }
        }        
        $field[$fname]['#'.$key] = ('false' === $value)? FALSE : $value;
        unset($field[$key]);
    }
    
    return $field;
}