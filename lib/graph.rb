#
# ideas for a graphing DSL
#

module RRDtool

  # Simple Ruby object to generate the RRD graph parameters
  #
  class Graph
    
    # Graph attributes can appear in any order
    attr :image_file, :image_format
    attr :title
    attr :vertical_label
    
    attr :width, :height
    attr :start, :duration, :step
    attr :upper_limit, :lower_limit
    
    # LATER: there are lots of other RRDtool graph attributes
    
    def initialize
      @graph_command = []
    end

    # 
    # The graph commands must be preserved in order
    #

    # Create a DEF
    def fetch(name, rrd_file, value, type)
      # format: DEF:name=rrd_file:value:type
      @graph_command << "DEF:#{name}=#{rrd_file}:#{value}:#{type}"
    end
    
    # Create a CDEF
    def compute(name, expr)
      # format: CDEF:name=expr
      @graph_command << "CDEF:#{name}=#{expr}"
    end
    
    # Create GPRINT
    def gprint(name, type, print_expr)
      # format: GPRINTF:name:type:print_expr
      @graph_command << "GPRINTF:#{name}:#{type}:#{print_expr}"    
    end
    
    # Create a LINE(1,2,...)
    def line(name, width, color, label)
      # format: LINEwidth:name#color:label
      @graph_command << "LINE#{width}:#{name}##{color}:#{label}"
    end
    
    # Create a AREA
    def area(name, color)
      # format: AREA:name#color
      @graph_command << "AREA:#{name}##{color}"
    end
    
    # Create a STACK
    def stack(name, color)
      # format: STACK:name#color
      @graph_command << "STACK:#{name}##{color}"
    end

    # Turn the graph definition into a real image
    def publish
      # LATER: add in the graph attributes -- 
      # i.e., turn the graph attribute hash into an array of [ '--key', 'value', ...]
      RRDtool.graph @graph_command
    end

  end

end
