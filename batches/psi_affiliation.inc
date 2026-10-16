<?php

require_once dirname(__FILE__) . '/csv_load.inc';

/**
 * Update PSI author affiliation on a publication from a CSV.
 *
 * Command flags:
 *  --command=change_psi_affiliation --command_options="csv=/path/batch_update.csv"
 */
class change_psi_affiliation extends batch_edit_mods{
    protected $update_dc = false;

    protected function edit_mxds(){
        $csv = isset($this->options['csv']) ? $this->options['csv'] : '';
        if($csv === '' || !is_readable($csv)) return $this->get_mxds_content();

        $pid = $this->get_pid();
        $pub_id = $pid;
        $colon = strrpos($pid, ':');
        if($colon !== false) $pub_id = substr($pid, $colon + 1);

        $rows = array();
        foreach(csv_load::rows($csv) as $row) {
            if($row['pub_id'] === $pub_id || $row['pub_id'] === $pid) $rows[$row['psi_author_id']] = $row;
        }
        if(!$rows) return $this->get_mxds_content();

        $this->set_dom();
        $this->set_xpath();
        $edited = false;
        $names = $this->get_xpath()->query('//mods:mods/mods:name [@type="personal"]');
        foreach ($names as $name){
            $author_id = null;
            foreach($name->getElementsByTagName('nameIdentifier') as $id){
                if($id->getAttribute('type') == 'authorId') $author_id = trim($id->nodeValue);
            }
            if($author_id === null) continue;

            $bare = $author_id;
            $colon = strrpos($author_id, ':');
            if($colon !== false) $bare = substr($author_id, $colon + 1);
            if(isset($rows[$bare])) $row = $rows[$bare];
            elseif (isset($rows[$author_id])) $row = $rows[$author_id];
            else continue;

            foreach(array('group', 'section', 'department', 'division') as $type){
                if($this->set_tagged($name, 'affiliation', $type, $row[$type])) $edited = true;
            }
            if($this->set_tagged($name, 'nameIdentifier', 'organizational unit id', $row['org_unit_id'])) $edited = true;
        }

        if($edited) return $this->get_dom()->saveXML();
        else return $this->get_mxds_content();
    }

    private function set_tagged($parent, $tag, $type, $value){
        $value = trim($value);
        $node = null;
        foreach($parent->getElementsByTagName($tag) as $candidate){
            if($candidate->getAttribute('type') == $type) $node = $candidate;
        }

        if($value === '') {
            if($node === null) return false;
            $node->parentNode->removeChild($node);
            return true;
        }
        if($node === null) {
            $node = $this->get_dom()->createElement($tag, $value);
            $attribute = $this->get_dom()->createAttribute('type');
            $attribute->value = $type;
            $node->appendChild($attribute);
            $parent->appendChild($node);
            return true;
        }
        if($node->nodeValue == $value) return false;

        $node->nodeValue = $value;
        
        return true;
    }
}
